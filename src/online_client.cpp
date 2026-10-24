#include "online_client.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace gw2::online {
namespace {

bool IsNameChar(char c, bool first) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return true;
  return !first && (c == ' ' || c == '_' || c == '.' || c == '-');
}

std::string_view Trim(std::string_view s) {
  constexpr std::string_view kSpace = " \t\r\n";
  const size_t first = s.find_first_not_of(kSpace);
  if (first == std::string_view::npos) return {};
  return s.substr(first, s.find_last_not_of(kSpace) - first + 1);
}

std::string Lower(std::string_view s) {
  std::string out(s);
  for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// localhost, 127.x, 10.x, 192.168.x and 172.16-31.x: a server on this PC or in a home network.
bool IsPrivateHost(std::string_view host) {
  if (host == "localhost" || StartsWith(host, "127.") || StartsWith(host, "10.") || StartsWith(host, "192.168.")) {
    return true;
  }
  if (StartsWith(host, "172.")) {
    const int second = std::atoi(std::string(host.substr(4)).c_str());
    return second >= 16 && second <= 31;
  }
  return false;
}

void AppendUtf8(std::string& out, unsigned code) {
  if (code < 0x80) {
    out += static_cast<char>(code);
  } else if (code < 0x800) {
    out += static_cast<char>(0xC0 | (code >> 6));
    out += static_cast<char>(0x80 | (code & 0x3F));
  } else {
    out += static_cast<char>(0xE0 | (code >> 12));
    out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (code & 0x3F));
  }
}

// A cursor over JSON text for ParseFlatJson.
class JsonReader {
 public:
  explicit JsonReader(std::string_view text) : text_(text) {}

  bool AtEnd() const { return pos_ >= text_.size(); }
  void SkipSpace() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
  }
  bool Take(char c) {
    SkipSpace();
    if (pos_ < text_.size() && text_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }
  char Peek() {
    SkipSpace();
    return pos_ < text_.size() ? text_[pos_] : '\0';
  }

  bool String(std::string& out) {
    out.clear();
    if (!Take('"')) return false;
    while (pos_ < text_.size()) {
      const char c = text_[pos_++];
      if (c == '"') return true;
      if (c != '\\') {
        out += c;
        continue;
      }
      if (pos_ >= text_.size()) return false;
      const char e = text_[pos_++];
      switch (e) {
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case 'r': out += '\r'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'u': {
          if (pos_ + 4 > text_.size()) return false;
          const std::string hex(text_.substr(pos_, 4));
          char* end = nullptr;
          const unsigned long code = std::strtoul(hex.c_str(), &end, 16);
          if (end != hex.c_str() + 4) return false;
          AppendUtf8(out, static_cast<unsigned>(code));
          pos_ += 4;
          break;
        }
        default: out += e; break;  // \" \\ \/
      }
    }
    return false;
  }

  // Skips one value of any kind, nested ones included.
  bool SkipValue() {
    const char c = Peek();
    if (c == '"') {
      std::string ignored;
      return String(ignored);
    }
    if (c == '{' || c == '[') {
      int depth = 0;
      while (pos_ < text_.size()) {
        const char d = text_[pos_];
        if (d == '"') {
          std::string ignored;
          if (!String(ignored)) return false;
          continue;
        }
        ++pos_;
        if (d == '{' || d == '[') ++depth;
        if (d == '}' || d == ']') {
          if (--depth == 0) return true;
        }
      }
      return false;
    }
    return !Literal().empty();
  }

  // A bare literal: number, true, false or null.
  std::string Literal() {
    SkipSpace();
    const size_t start = pos_;
    while (pos_ < text_.size() && text_[pos_] != ',' && text_[pos_] != '}' && text_[pos_] != ']' &&
           !std::isspace(static_cast<unsigned char>(text_[pos_]))) {
      ++pos_;
    }
    return std::string(text_.substr(start, pos_ - start));
  }

 private:
  std::string_view text_;
  size_t pos_ = 0;
};

std::string Detail(const HttpResponse& response) {
  const auto json = ParseFlatJson(response.body);
  const auto it = json.find("detail");
  return it == json.end() ? std::string() : it->second;
}

}  // namespace

std::string NameProblem(std::string_view name) {
  if (name.empty()) return "Enter a name.";
  if (name.size() > kMaxNameLength) return "At most 15 characters.";
  for (size_t i = 0; i < name.size(); ++i) {
    if (!IsNameChar(name[i], i == 0)) {
      return i == 0 ? "Start with a letter or a digit."
                    : "Use letters, digits, space, underscore, dot or hyphen only.";
    }
  }
  if (name.back() == ' ') return "No space at the end.";
  return {};
}

std::string NormalizeBaseUrl(std::string_view url) {
  std::string_view text = Trim(url);
  if (text.empty() || text.find_first_of(" \t\r\n?#") != std::string_view::npos) return {};
  const std::string lower = Lower(text);
  bool secure = false;
  if (StartsWith(lower, "https://")) {
    secure = true;
  } else if (!StartsWith(lower, "http://")) {
    return {};
  }
  const size_t host_start = secure ? 8 : 7;
  size_t host_end = text.find_first_of(":/", host_start);
  if (host_end == std::string_view::npos) host_end = text.size();
  const std::string host = Lower(text.substr(host_start, host_end - host_start));
  if (host.empty()) return {};
  if (!secure && !IsPrivateHost(host)) return {};
  std::string out(text);
  while (!out.empty() && out.back() == '/') out.pop_back();
  // The scheme is written in lower case; the rest stays as typed.
  out.replace(0, host_start, secure ? "https://" : "http://");
  return out;
}

std::string JsonEscape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 2);
  for (const unsigned char c : text) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

std::string UrlEncode(std::string_view text) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  for (const unsigned char c : text) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += kHex[c >> 4];
      out += kHex[c & 15];
    }
  }
  return out;
}

std::map<std::string, std::string> ParseFlatJson(std::string_view json) {
  std::map<std::string, std::string> out;
  JsonReader in(json);
  if (!in.Take('{')) return out;
  while (!in.AtEnd()) {
    if (in.Take('}')) break;
    std::string key;
    if (!in.String(key) || !in.Take(':')) return out;
    const char first = in.Peek();
    if (first == '"') {
      std::string value;
      if (!in.String(value)) return out;
      out[key] = value;
    } else if (first == '{' || first == '[') {
      if (!in.SkipValue()) return out;
    } else {
      const std::string literal = in.Literal();
      if (literal.empty()) return out;
      if (literal != "null") out[key] = literal;
    }
    in.Take(',');
  }
  return out;
}

std::string DescribeError(AccountError error) {
  switch (error) {
    case AccountError::kNone: return {};
    case AccountError::kInvalidName: return "That name is not allowed.";
    case AccountError::kNameTaken: return "That name is already taken.";
    case AccountError::kClosed: return "The server is not accepting new players.";
    case AccountError::kRefused: return "The server refused that.";
    case AccountError::kRateLimited: return "Too many attempts. Try again later.";
    case AccountError::kBadToken: return "Your account is no longer known to the server.";
    case AccountError::kUnreachable: return "The server could not be reached.";
    case AccountError::kServer: return "The server gave an unexpected answer.";
  }
  return {};
}

int RetryDelayMs(int attempt, int first_ms, int max_ms) {
  long long delay = first_ms;
  for (int i = 0; i < attempt && delay < max_ms; ++i) delay *= 2;
  return static_cast<int>(std::min<long long>(delay, max_ms));
}

NameCheck CheckName(HttpTransport& transport, const std::string& base_url, std::string_view name) {
  if (const std::string problem = NameProblem(name); !problem.empty()) return {NameState::kInvalid, problem};
  if (base_url.empty()) return {NameState::kUnreachable, "No server address is set."};

  HttpRequest request;
  request.method = "GET";
  request.url = base_url + "/v1/name-available?name=" + UrlEncode(name);
  const HttpResponse response = transport.Send(request);
  if (response.status == 429) return {NameState::kUnreachable, "Too many checks, wait a moment."};
  if (response.status != 200) return {NameState::kUnreachable, "The server could not be reached."};

  const auto json = ParseFlatJson(response.body);
  const auto available = json.find("available");
  if (available == json.end()) return {NameState::kUnreachable, "The server gave an unexpected answer."};
  if (available->second == "true") return {NameState::kAvailable, {}};
  const auto reason = json.find("reason");
  if (reason != json.end() && reason->second == "taken") return {NameState::kTaken, "That name is already taken."};
  const auto message = json.find("message");
  return {NameState::kInvalid, message != json.end() ? message->second : "That name is not allowed."};
}

RegisterResult Register(HttpTransport& transport, const std::string& base_url, std::string_view name) {
  RegisterResult result;
  if (const std::string problem = NameProblem(name); !problem.empty()) {
    result.error = AccountError::kInvalidName;
    result.message = problem;
    return result;
  }
  HttpRequest request;
  request.method = "POST";
  request.url = base_url + "/v1/register";
  request.body = "{\"name\":\"" + JsonEscape(name) + "\"}";
  const HttpResponse response = transport.Send(request);

  switch (response.status) {
    case 201: {
      const auto json = ParseFlatJson(response.body);
      const auto token = json.find("token");
      const auto xuid = json.find("xuid");
      const auto echoed = json.find("name");
      if (token == json.end() || token->second.empty()) {
        result.error = AccountError::kServer;
        break;
      }
      result.error = AccountError::kNone;
      result.account = {echoed != json.end() ? echoed->second : std::string(name), token->second,
                        xuid != json.end() ? xuid->second : std::string()};
      return result;
    }
    case 0: result.error = AccountError::kUnreachable; break;
    case 403: result.error = AccountError::kClosed; break;
    case 409: result.error = AccountError::kNameTaken; break;
    case 422:
      result.error = AccountError::kInvalidName;
      result.message = Detail(response);
      break;
    case 429: result.error = AccountError::kRateLimited; break;
    default: result.error = AccountError::kServer; break;
  }
  if (result.message.empty()) result.message = DescribeError(result.error);
  return result;
}

RegisterResult Restore(HttpTransport& transport, const std::string& base_url, const std::string& token) {
  RegisterResult result;
  if (token.empty() || token.size() > 200 || token.find_first_of(" \t\r\n\"") != std::string::npos) {
    result.error = AccountError::kBadToken;
    result.message = "That is not an account key.";
    return result;
  }
  HttpRequest request;
  request.method = "GET";
  request.url = base_url + "/v1/me";
  request.bearer = token;
  const HttpResponse response = transport.Send(request);
  switch (response.status) {
    case 200: {
      const auto json = ParseFlatJson(response.body);
      const auto name = json.find("name");
      const auto xuid = json.find("xuid");
      if (name == json.end() || name->second.empty()) {
        result.error = AccountError::kServer;
        break;
      }
      result.error = AccountError::kNone;
      result.account = {name->second, token, xuid != json.end() ? xuid->second : std::string()};
      return result;
    }
    case 0: result.error = AccountError::kUnreachable; break;
    case 401: result.error = AccountError::kBadToken; break;
    case 429: result.error = AccountError::kRateLimited; break;
    default: result.error = AccountError::kServer; break;
  }
  result.message = result.error == AccountError::kBadToken ? "The server does not know this account key."
                                                           : DescribeError(result.error);
  return result;
}

RenameResult Rename(HttpTransport& transport, const std::string& base_url, const std::string& token,
                    std::string_view name) {
  RenameResult result;
  if (const std::string problem = NameProblem(name); !problem.empty()) {
    result.error = AccountError::kInvalidName;
    result.message = problem;
    return result;
  }
  HttpRequest request;
  request.method = "PUT";
  request.url = base_url + "/v1/me";
  request.bearer = token;
  request.body = "{\"name\":\"" + JsonEscape(name) + "\"}";
  const HttpResponse response = transport.Send(request);
  switch (response.status) {
    case 200: result.error = AccountError::kNone; return result;
    case 0: result.error = AccountError::kUnreachable; break;
    case 401: result.error = AccountError::kBadToken; break;
    case 409: result.error = AccountError::kNameTaken; break;
    case 422:
      result.error = AccountError::kInvalidName;
      result.message = Detail(response);
      break;
    default: result.error = AccountError::kServer; break;
  }
  if (result.message.empty()) result.message = DescribeError(result.error);
  return result;
}

namespace {

// The error of a friend call from its status. 404, 409, 422 and 429 carry a sentence of the server's for the player.
ActionResult FriendFailure(const HttpResponse& response) {
  ActionResult result;
  switch (response.status) {
    case 0: result.error = AccountError::kUnreachable; break;
    case 401: result.error = AccountError::kBadToken; break;
    case 429: result.error = AccountError::kRateLimited; break;
    case 404:
    case 409:
    case 422: result.error = AccountError::kRefused; break;
    default: result.error = AccountError::kServer; break;
  }
  if (result.error == AccountError::kRefused) result.message = Detail(response);
  if (result.message.empty()) result.message = DescribeError(result.error);
  return result;
}

HttpRequest AuthorizedPost(const std::string& base_url, const std::string& path, const std::string& token,
                           std::string body) {
  HttpRequest request;
  request.method = "POST";
  request.url = base_url + path;
  request.bearer = token;
  request.body = std::move(body);
  return request;
}

}  // namespace

ActionResult DeleteAccount(HttpTransport& transport, const std::string& base_url, const std::string& token) {
  const HttpResponse response =
      transport.Send(AuthorizedPost(base_url, "/v1/me/delete", token, R"({"confirm":"DELETE"})"));
  if (response.status == 200) {
    ActionResult done;
    done.error = AccountError::kNone;
    return done;
  }
  return FriendFailure(response);
}

std::string FriendTarget(std::string_view text) {
  const std::string_view trimmed = Trim(text);
  if (trimmed.empty() || trimmed.size() > 40) return {};
  return std::string(trimmed);
}

FriendRequestResult SendFriendRequest(HttpTransport& transport, const std::string& base_url, const std::string& token,
                                      std::string_view name) {
  FriendRequestResult result;
  const std::string target = FriendTarget(name);
  if (target.empty()) {
    result.error = AccountError::kRefused;
    result.message = "Enter the name of a player.";
    return result;
  }
  const HttpResponse response =
      transport.Send(AuthorizedPost(base_url, "/v1/friends/request", token, "{\"to\":\"" + JsonEscape(target) + "\"}"));
  if (response.status == 200 || response.status == 201) {
    const auto json = ParseFlatJson(response.body);
    const auto status = json.find("status");
    if (status != json.end() && (status->second == "sent" || status->second == "friends")) {
      const auto xuid = json.find("xuid");
      const auto name = json.find("name");
      result.error = AccountError::kNone;
      result.friends = status->second == "friends";
      result.person = {xuid != json.end() ? xuid->second : std::string(), name != json.end() ? name->second : std::string()};
      return result;
    }
    result.error = AccountError::kServer;
    result.message = DescribeError(result.error);
    return result;
  }
  static_cast<ActionResult&>(result) = FriendFailure(response);
  return result;
}

ActionResult AnswerFriend(HttpTransport& transport, const std::string& base_url, const std::string& token,
                          FriendAction action, std::string_view xuid) {
  const char* path = "/v1/friends/accept";
  switch (action) {
    case FriendAction::kAccept: path = "/v1/friends/accept"; break;
    case FriendAction::kDecline: path = "/v1/friends/decline"; break;
    case FriendAction::kCancel: path = "/v1/friends/cancel"; break;
    case FriendAction::kRemove: path = "/v1/friends/remove"; break;
  }
  const HttpResponse response =
      transport.Send(AuthorizedPost(base_url, path, token, "{\"xuid\":\"" + JsonEscape(xuid) + "\"}"));
  if (response.status == 200) {
    ActionResult done;
    done.error = AccountError::kNone;
    return done;
  }
  return FriendFailure(response);
}

UploadResult PostScores(HttpTransport& transport, const std::string& base_url, const std::string& token,
                        const std::map<uint32_t, int64_t>& scores, std::string* message) {
  std::string body = "{\"scores\":[";
  bool first = true;
  for (const auto& [view, score] : scores) {
    if (!first) body += ',';
    first = false;
    body += "{\"view\":" + std::to_string(view) + ",\"score\":" + std::to_string(score) + "}";
  }
  body += "]}";

  HttpRequest request;
  request.method = "POST";
  request.url = base_url + "/v1/scores";
  request.bearer = token;
  request.body = std::move(body);
  const HttpResponse response = transport.Send(request);

  if (response.status == 200) return UploadResult::kOk;
  if (message) {
    *message = response.status == 0 ? (response.error.empty() ? "no connection" : response.error)
                                    : "server answered " + std::to_string(response.status);
  }
  if (response.status == 0 || response.status == 429 || response.status >= 500) return UploadResult::kRetry;
  return UploadResult::kRejected;  // 401 (unknown token), 422 (the request itself), anything else in 4xx
}

OnlineClient::OnlineClient(ClientConfig config, std::unique_ptr<HttpTransport> transport, Snapshot snapshot, Log log,
                           ClientTiming timing, std::function<void()> on_uploaded)
    : config_(std::move(config)),
      transport_(std::move(transport)),
      snapshot_(std::move(snapshot)),
      log_(std::move(log)),
      timing_(timing),
      on_uploaded_(std::move(on_uploaded)) {
  worker_ = std::thread([this] { Run(); });
}

OnlineClient::~OnlineClient() {
  {
    std::lock_guard lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  if (worker_.joinable()) worker_.join();
}

void OnlineClient::ScoresChanged() {
  {
    std::lock_guard lock(mutex_);
    if (rejected_) return;
    dirty_ = true;
    nudged_ = true;
  }
  wake_.notify_all();
}

bool OnlineClient::Rejected() const {
  std::lock_guard lock(mutex_);
  return rejected_;
}

int OnlineClient::Uploads() const {
  std::lock_guard lock(mutex_);
  return uploads_;
}

void OnlineClient::Run() {
  std::unique_lock lock(mutex_);
  int failures = 0;
  while (!stop_) {
    wake_.wait(lock, [this] { return stop_ || dirty_; });
    if (stop_) break;
    dirty_ = false;
    nudged_ = false;
    lock.unlock();

    const std::map<uint32_t, int64_t> scores = snapshot_ ? snapshot_() : std::map<uint32_t, int64_t>{};
    std::string why;
    const UploadResult result =
        scores.empty() ? UploadResult::kOk : PostScores(*transport_, config_.url, config_.token, scores, &why);
    if (result == UploadResult::kOk && !scores.empty() && on_uploaded_) on_uploaded_();

    lock.lock();
    if (result == UploadResult::kOk) {
      if (!scores.empty()) ++uploads_;
      failures = 0;
      continue;
    }
    if (result == UploadResult::kRejected) {
      rejected_ = true;
      if (log_) log_("Online scores are off for this session: the server refused the account (" + why + ")");
      break;
    }
    dirty_ = true;
    const int delay = RetryDelayMs(failures, timing_.first_retry_ms, timing_.max_retry_ms);
    if (failures == 0 && log_) log_("Could not upload scores (" + why + "), will retry");
    ++failures;
    // A new score ends the pause early: it should not wait for the old backoff.
    wake_.wait_for(lock, std::chrono::milliseconds(delay), [this] { return stop_ || nudged_; });
  }
}

}  // namespace gw2::online
