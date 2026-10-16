#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

// The friends leaderboard: a small HTTPS API (see the server's README). Nothing here touches the runtime, so the
// whole file is covered by tests\online_tests.cpp with a fake transport.
namespace gw2::online {

constexpr size_t kMaxNameLength = 15;  // the game's gamertag buffer is 16 bytes including the terminator

// Why an online name is not acceptable, or an empty string when it is. The server applies the same rules.
std::string NameProblem(std::string_view name);

// "https://host[:port][/prefix]" without a trailing slash, or an empty string when the address is unusable. Plain
// http is accepted only for a server on this PC or in a private network, for testing.
std::string NormalizeBaseUrl(std::string_view url);

std::string JsonEscape(std::string_view text);
std::string UrlEncode(std::string_view text);

// The members of a flat JSON object that are strings, booleans or numbers (as text). Anything else is skipped.
// Enough for the server's small answers; not a general JSON parser.
std::map<std::string, std::string> ParseFlatJson(std::string_view json);

struct HttpRequest {
  std::string method;  // GET, POST, PUT
  std::string url;
  std::string body;  // JSON, empty for none
  std::string bearer;
  int timeout_ms = 8000;
  std::string if_none_match;  // an ETag from an earlier answer, empty for none
};

struct HttpResponse {
  int status = 0;  // 0: no answer at all (see error)
  std::string body;
  std::string error;
  std::string etag;  // the answer's ETag header, when it has one
};

class HttpTransport {
 public:
  virtual ~HttpTransport() = default;
  virtual HttpResponse Send(const HttpRequest& request) = 0;
};

// WinHTTP on Windows. Other platforms get a transport that always fails with an explanation.
std::unique_ptr<HttpTransport> MakeSystemTransport();

// -- Calls. All of them block, so they belong on a worker thread.

enum class NameState { kAvailable, kTaken, kInvalid, kUnreachable };

struct NameCheck {
  NameState state = NameState::kUnreachable;
  std::string message;
};

// The rules are checked here first, so a bad name never costs a request.
NameCheck CheckName(HttpTransport& transport, const std::string& base_url, std::string_view name);

enum class AccountError {
  kNone,
  kInvalidName,
  kNameTaken,
  kWrongInvite,
  kClosed,
  kRateLimited,
  kBadToken,
  kUnreachable,
  kServer,
};

struct Account {
  std::string name;
  std::string token;
  std::string xuid;
};

struct RegisterResult {
  AccountError error = AccountError::kServer;
  std::string message;
  Account account;
  bool ok() const { return error == AccountError::kNone; }
};

RegisterResult Register(HttpTransport& transport, const std::string& base_url, std::string_view name,
                        std::string_view invite);

struct RenameResult {
  AccountError error = AccountError::kServer;
  std::string message;
  bool ok() const { return error == AccountError::kNone; }
};

RenameResult Rename(HttpTransport& transport, const std::string& base_url, const std::string& token,
                    std::string_view name);

enum class UploadResult {
  kOk,
  kRetry,     // no connection, a busy or failing server: try again later
  kRejected,  // the server refuses this token or request; trying again cannot help
};

UploadResult PostScores(HttpTransport& transport, const std::string& base_url, const std::string& token,
                        const std::map<uint32_t, int64_t>& scores, std::string* message = nullptr);

// A short, plain sentence for the launcher.
std::string DescribeError(AccountError error);

// Delay before retry number `attempt` (0 is the first retry): doubles from `first_ms` up to `max_ms`.
int RetryDelayMs(int attempt, int first_ms, int max_ms);

// -- The background upload of the player's clean best scores.

struct ClientConfig {
  std::string url;    // normalized
  std::string token;
};

struct ClientTiming {
  int first_retry_ms = 15000;
  int max_retry_ms = 15 * 60 * 1000;
};

// Uploads the scores the snapshot returns whenever they may have changed: once at start, then after every
// ScoresChanged(). The server keeps only the better value, so an upload is safe to repeat. A server that cannot be
// reached is retried with growing pauses; a rejected token ends the uploads for this run. The game's threads only
// ever call ScoresChanged(), which never blocks on the network.
class OnlineClient {
 public:
  using Snapshot = std::function<std::map<uint32_t, int64_t>()>;
  using Log = std::function<void(const std::string&)>;

  // `on_uploaded` runs on the worker thread after every upload the server accepted.
  OnlineClient(ClientConfig config, std::unique_ptr<HttpTransport> transport, Snapshot snapshot, Log log,
               ClientTiming timing = {}, std::function<void()> on_uploaded = {});
  ~OnlineClient();
  OnlineClient(const OnlineClient&) = delete;
  OnlineClient& operator=(const OnlineClient&) = delete;

  void ScoresChanged();

  // For tests and diagnostics.
  bool Rejected() const;
  int Uploads() const;

 private:
  void Run();

  ClientConfig config_;
  std::unique_ptr<HttpTransport> transport_;
  Snapshot snapshot_;
  Log log_;
  ClientTiming timing_;
  std::function<void()> on_uploaded_;

  mutable std::mutex mutex_;
  std::condition_variable wake_;
  bool stop_ = false;
  bool dirty_ = true;  // the first pass uploads everything the snapshot holds
  bool nudged_ = false;
  bool rejected_ = false;
  int uploads_ = 0;
  std::thread worker_;
};

}  // namespace gw2::online
