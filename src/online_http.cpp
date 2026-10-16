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

class UnsupportedTransport final : public HttpTransport {
 public:
  HttpResponse Send(const HttpRequest&) override {
    HttpResponse out;
    out.error = "online features are not available on this platform yet";
    return out;
  }
};

#endif

}  // namespace

std::unique_ptr<HttpTransport> MakeSystemTransport() {
#if defined(_WIN32)
  return std::make_unique<WinHttpTransport>();
#else
  return std::make_unique<UnsupportedTransport>();
#endif
}

}  // namespace gw2::online
