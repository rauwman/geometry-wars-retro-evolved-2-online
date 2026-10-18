#include "online_client.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#else
#include <dlfcn.h>

#include <cctype>
#include <cstring>
#include <mutex>
#endif

namespace gw2::online {
namespace {

#if defined(_WIN32)

constexpr size_t kMaxBody = 1u << 20;

std::wstring Widen(const std::string& text) {
  if (text.empty()) return {};
  const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring out(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
  return out;
}

// Closes a WinHTTP handle when it goes out of scope.
class Handle {
 public:
  explicit Handle(HINTERNET h = nullptr) : h_(h) {}
  ~Handle() {
    if (h_) WinHttpCloseHandle(h_);
  }
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  explicit operator bool() const { return h_ != nullptr; }
  HINTERNET get() const { return h_; }

 private:
  HINTERNET h_;
};

std::string DescribeWinHttpError(DWORD code) {
  switch (code) {
    case ERROR_WINHTTP_TIMEOUT: return "the server did not answer in time";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "the server name could not be resolved";
    case ERROR_WINHTTP_CANNOT_CONNECT: return "could not connect to the server";
    case ERROR_WINHTTP_CONNECTION_ERROR: return "the connection was lost";
    case ERROR_WINHTTP_SECURE_FAILURE: return "the HTTPS certificate was not accepted";
    default: return "WinHTTP error " + std::to_string(code);
  }
}

class WinHttpTransport final : public HttpTransport {
 public:
  HttpResponse Send(const HttpRequest& request) override {
    HttpResponse out;
    const std::wstring url = Widen(request.url);

    URL_COMPONENTSW parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = DWORD(-1);
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts)) {
      out.error = "bad server address";
      return out;
    }
    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    const std::wstring target = std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) +
                                std::wstring(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    const bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;

    Handle session(WinHttpOpen(L"GeometryWarsRetroEvolved2-PC-Port", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) return Fail(out);
    WinHttpSetTimeouts(session.get(), request.timeout_ms, request.timeout_ms, request.timeout_ms,
                       request.timeout_ms);
    Handle connection(WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0));
    if (!connection) return Fail(out);
    Handle http(WinHttpOpenRequest(connection.get(), Widen(request.method).c_str(), target.c_str(), nullptr,
                                   WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                   secure ? WINHTTP_FLAG_SECURE : 0));
    if (!http) return Fail(out);

    std::wstring headers = L"Accept: application/json\r\n";
    if (!request.body.empty()) headers += L"Content-Type: application/json\r\n";
    if (!request.bearer.empty()) headers += L"Authorization: Bearer " + Widen(request.bearer) + L"\r\n";
    if (!request.if_none_match.empty()) headers += L"If-None-Match: " + Widen(request.if_none_match) + L"\r\n";

    void* body = request.body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(request.body.data());
    const DWORD body_size = static_cast<DWORD>(request.body.size());
    if (!WinHttpSendRequest(http.get(), headers.c_str(), DWORD(-1), body, body_size, body_size, 0) ||
        !WinHttpReceiveResponse(http.get(), nullptr)) {
      return Fail(out);
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(http.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX)) {
      return Fail(out);
    }
    out.status = static_cast<int>(status);

    wchar_t etag[128] = {};
    DWORD etag_size = sizeof(etag) - sizeof(wchar_t);
    if (WinHttpQueryHeaders(http.get(), WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX, etag, &etag_size,
                            WINHTTP_NO_HEADER_INDEX)) {
      out.etag.reserve(etag_size / sizeof(wchar_t));
      for (DWORD i = 0; i < etag_size / sizeof(wchar_t); ++i) out.etag += static_cast<char>(etag[i] & 0x7F);
    }

    for (;;) {
      DWORD available = 0;
      if (!WinHttpQueryDataAvailable(http.get(), &available) || available == 0) break;
      if (out.body.size() + available > kMaxBody) {
        out.error = "answer too large";
        out.status = 0;
        return out;
      }
      const size_t at = out.body.size();
      out.body.resize(at + available);
      DWORD read = 0;
      if (!WinHttpReadData(http.get(), out.body.data() + at, available, &read)) return Fail(out);
      out.body.resize(at + read);
    }
    return out;
  }

 private:
  static HttpResponse Fail(HttpResponse& out) {
    out.status = 0;
    out.error = DescribeWinHttpError(GetLastError());
    return out;
  }
};

#else

constexpr size_t kMaxBody = 1u << 20;

// libcurl is loaded at run time (dlopen), so the program builds and starts without it and needs no curl headers.
// Only the stable libcurl.so.4 ABI is used; the constants below are the documented curl.h values.
using CurlEasy = void;
using CurlSlist = void;
using WriteFn = size_t (*)(char*, size_t, size_t, void*);

constexpr int kCurlOptWriteData = 10001;
constexpr int kCurlOptUrl = 10002;
constexpr int kCurlOptPostFields = 10015;
constexpr int kCurlOptUserAgent = 10018;
constexpr int kCurlOptHttpHeader = 10023;
constexpr int kCurlOptHeaderData = 10029;
constexpr int kCurlOptCustomRequest = 10036;
constexpr int kCurlOptPostFieldSize = 60;
constexpr int kCurlOptNoSignal = 99;
constexpr int kCurlOptTimeoutMs = 155;
constexpr int kCurlOptConnectTimeoutMs = 156;
constexpr int kCurlOptWriteFunction = 20011;
constexpr int kCurlOptHeaderFunction = 20079;
constexpr int kCurlInfoResponseCode = 0x200002;
constexpr int kCurlGlobalDefault = 3;

struct CurlApi {
  int (*global_init)(long) = nullptr;
  CurlEasy* (*easy_init)() = nullptr;
  int (*easy_setopt)(CurlEasy*, int, ...) = nullptr;
  int (*easy_perform)(CurlEasy*) = nullptr;
  int (*easy_getinfo)(CurlEasy*, int, ...) = nullptr;
  void (*easy_cleanup)(CurlEasy*) = nullptr;
  CurlSlist* (*slist_append)(CurlSlist*, const char*) = nullptr;
  void (*slist_free_all)(CurlSlist*) = nullptr;
  const char* (*easy_strerror)(int) = nullptr;
  bool ok = false;
};

const CurlApi& Curl() {
  static CurlApi api;
  static std::once_flag once;
  std::call_once(once, [] {
    void* lib = nullptr;
    for (const char* name : {"libcurl.so.4", "libcurl-gnutls.so.4", "libcurl-nss.so.4"}) {
      lib = dlopen(name, RTLD_NOW | RTLD_LOCAL);
      if (lib) break;
    }
    if (!lib) return;
    auto sym = [&](const char* name) { return dlsym(lib, name); };
    api.global_init = reinterpret_cast<decltype(api.global_init)>(sym("curl_global_init"));
    api.easy_init = reinterpret_cast<decltype(api.easy_init)>(sym("curl_easy_init"));
    api.easy_setopt = reinterpret_cast<decltype(api.easy_setopt)>(sym("curl_easy_setopt"));
    api.easy_perform = reinterpret_cast<decltype(api.easy_perform)>(sym("curl_easy_perform"));
    api.easy_getinfo = reinterpret_cast<decltype(api.easy_getinfo)>(sym("curl_easy_getinfo"));
    api.easy_cleanup = reinterpret_cast<decltype(api.easy_cleanup)>(sym("curl_easy_cleanup"));
    api.slist_append = reinterpret_cast<decltype(api.slist_append)>(sym("curl_slist_append"));
    api.slist_free_all = reinterpret_cast<decltype(api.slist_free_all)>(sym("curl_slist_free_all"));
    api.easy_strerror = reinterpret_cast<decltype(api.easy_strerror)>(sym("curl_easy_strerror"));
    api.ok = api.global_init && api.easy_init && api.easy_setopt && api.easy_perform && api.easy_getinfo &&
             api.easy_cleanup && api.slist_append && api.slist_free_all && api.easy_strerror;
    if (api.ok) api.global_init(kCurlGlobalDefault);
  });
  return api;
}

std::string DescribeCurlError(const CurlApi& api, int code) {
  switch (code) {
    case 5:
    case 6: return "the server name could not be resolved";
    case 7: return "could not connect to the server";
    case 28: return "the server did not answer in time";
    case 23: return "answer too large";
    case 35:
    case 51:
    case 58:
    case 60:
    case 77:
    case 83: return "the HTTPS certificate was not accepted";
    case 52:
    case 56: return "the connection was lost";
    default: return std::string("curl: ") + api.easy_strerror(code);
  }
}

size_t WriteBody(char* data, size_t size, size_t count, void* user) {
  auto* body = static_cast<std::string*>(user);
  const size_t bytes = size * count;
  if (body->size() + bytes > kMaxBody) return 0;  // makes curl fail with "write error"
  body->append(data, bytes);
  return bytes;
}

// Keeps the ETag header value.
size_t WriteHeader(char* data, size_t size, size_t count, void* user) {
  auto* etag = static_cast<std::string*>(user);
  const size_t bytes = size * count;
  constexpr char kName[] = "etag:";
  constexpr size_t kNameSize = sizeof(kName) - 1;
  if (bytes > kNameSize) {
    bool match = true;
    for (size_t i = 0; i < kNameSize; ++i) {
      if (std::tolower(static_cast<unsigned char>(data[i])) != kName[i]) {
        match = false;
        break;
      }
    }
    if (match) {
      size_t begin = kNameSize;
      size_t end = bytes;
      while (begin < end && (data[begin] == ' ' || data[begin] == '\t')) ++begin;
      while (end > begin && (data[end - 1] == '\r' || data[end - 1] == '\n' || data[end - 1] == ' ')) --end;
      etag->assign(data + begin, end - begin);
    }
  }
  return bytes;
}

class CurlTransport final : public HttpTransport {
 public:
  HttpResponse Send(const HttpRequest& request) override {
    HttpResponse out;
    const CurlApi& api = Curl();
    if (!api.ok) {
      out.error = "libcurl.so.4 was not found; install your distribution's libcurl package to use the online features";
      return out;
    }
    if (request.url.compare(0, 8, "https://") != 0 && request.url.compare(0, 7, "http://") != 0) {
      out.error = "bad server address";
      return out;
    }
    CurlEasy* easy = api.easy_init();
    if (!easy) {
      out.error = "could not start libcurl";
      return out;
    }

    CurlSlist* headers = api.slist_append(nullptr, "Accept: application/json");
    if (!request.body.empty()) headers = api.slist_append(headers, "Content-Type: application/json");
    if (!request.bearer.empty()) headers = api.slist_append(headers, ("Authorization: Bearer " + request.bearer).c_str());
    if (!request.if_none_match.empty()) {
      headers = api.slist_append(headers, ("If-None-Match: " + request.if_none_match).c_str());
    }

    std::string body;
    std::string etag;
    const long timeout = request.timeout_ms > 0 ? static_cast<long>(request.timeout_ms) : 10000L;
    api.easy_setopt(easy, kCurlOptUrl, request.url.c_str());
    api.easy_setopt(easy, kCurlOptCustomRequest, request.method.c_str());
    api.easy_setopt(easy, kCurlOptHttpHeader, headers);
    api.easy_setopt(easy, kCurlOptUserAgent, "GeometryWarsRetroEvolved2-PC-Port");
    api.easy_setopt(easy, kCurlOptNoSignal, 1L);
    api.easy_setopt(easy, kCurlOptTimeoutMs, timeout);
    api.easy_setopt(easy, kCurlOptConnectTimeoutMs, timeout);
    api.easy_setopt(easy, kCurlOptWriteFunction, static_cast<WriteFn>(WriteBody));
    api.easy_setopt(easy, kCurlOptWriteData, &body);
    api.easy_setopt(easy, kCurlOptHeaderFunction, static_cast<WriteFn>(WriteHeader));
    api.easy_setopt(easy, kCurlOptHeaderData, &etag);
    if (!request.body.empty()) {
      api.easy_setopt(easy, kCurlOptPostFields, request.body.data());
      api.easy_setopt(easy, kCurlOptPostFieldSize, static_cast<long>(request.body.size()));
    }

    const int code = api.easy_perform(easy);
    long status = 0;
    if (code == 0) api.easy_getinfo(easy, kCurlInfoResponseCode, &status);
    api.easy_cleanup(easy);
    api.slist_free_all(headers);

    if (code != 0) {
      out.error = DescribeCurlError(api, code);
      return out;
    }
    out.status = static_cast<int>(status);
    out.body = std::move(body);
    out.etag = std::move(etag);
    return out;
  }
};

#endif

}  // namespace

std::unique_ptr<HttpTransport> MakeSystemTransport() {
#if defined(_WIN32)
  return std::make_unique<WinHttpTransport>();
#else
  return std::make_unique<CurlTransport>();
#endif
}

}  // namespace gw2::online
