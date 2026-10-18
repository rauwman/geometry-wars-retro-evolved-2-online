// Tests of the friends leaderboard client: the name rules, the server address, the small JSON reader, the calls and
// the background upload, over a fake transport. Needs no game files, no network and no ReXGlue.
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "online_client.h"

using namespace gw2::online;
using namespace std::chrono_literals;

static int g_failures = 0;
static void Check(bool ok, const std::string& what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}
static void Section(const char* name) { std::printf("\n== %s\n", name); }

// Answers requests from a script and remembers them.
class FakeTransport final : public HttpTransport {
 public:
  using Handler = std::function<HttpResponse(const HttpRequest&, int call)>;
  explicit FakeTransport(Handler handler, std::shared_ptr<std::vector<HttpRequest>> log = nullptr,
                         std::shared_ptr<std::mutex> mutex = nullptr)
      : handler_(std::move(handler)),
        log_(log ? log : std::make_shared<std::vector<HttpRequest>>()),
        mutex_(mutex ? mutex : std::make_shared<std::mutex>()) {}

  HttpResponse Send(const HttpRequest& request) override {
    int call;
    {
      std::lock_guard lock(*mutex_);
      log_->push_back(request);
      call = static_cast<int>(log_->size());
    }
    return handler_(request, call);
  }

  std::vector<HttpRequest> Requests() const {
    std::lock_guard lock(*mutex_);
    return *log_;
  }

 private:
  Handler handler_;
  std::shared_ptr<std::vector<HttpRequest>> log_;
  std::shared_ptr<std::mutex> mutex_;
};

static HttpResponse Answer(int status, std::string body = {}) {
  HttpResponse r;
  r.status = status;
  r.body = std::move(body);
  return r;
}

static bool WaitFor(const std::function<bool()>& condition, std::chrono::milliseconds limit = 3000ms) {
  const auto end = std::chrono::steady_clock::now() + limit;
  while (std::chrono::steady_clock::now() < end) {
    if (condition()) return true;
    std::this_thread::sleep_for(5ms);
  }
  return condition();
}

static void TestNames() {
  Section("name rules");
  for (const char* ok : {"Yann", "Yann_R", "Yann R", "a", "7up", "Max.Muster-1", "123456789012345"}) {
    Check(NameProblem(ok).empty(), std::string("accepted: ") + ok);
  }
  // The same cases the server's tests reject.
  for (const std::string& bad : {std::string(), std::string(" Yann"), std::string("Yann "), std::string(16, 'x'),
                                std::string("\xC3\x9C" "nal"), std::string("a/b"), std::string("ab\n"),
                                std::string(".x"), std::string("-x"), std::string("_x")}) {
    Check(!NameProblem(bad).empty(), "rejected: \"" + bad + "\"");
  }
  Check(NameProblem("").find("Enter") != std::string::npos, "an empty name asks for one");
  Check(NameProblem("Yann ").find("end") != std::string::npos, "a trailing space is explained");
}

static void TestBaseUrl() {
  Section("server address");
  Check(NormalizeBaseUrl("https://gw2.rauwman.org") == "https://gw2.rauwman.org", "plain https address");
  Check(NormalizeBaseUrl("  https://gw2.rauwman.org/  ") == "https://gw2.rauwman.org", "trimmed, no trailing slash");
  Check(NormalizeBaseUrl("HTTPS://Host.Example/api//") == "https://Host.Example/api", "scheme lowered, prefix kept");
  Check(NormalizeBaseUrl("https://host:8443") == "https://host:8443", "port kept");
  Check(NormalizeBaseUrl("http://example.com").empty(), "plain http to the internet is refused");
  Check(NormalizeBaseUrl("http://localhost:8080") == "http://localhost:8080", "http to localhost");
  Check(NormalizeBaseUrl("http://192.168.20.254:8087") == "http://192.168.20.254:8087", "http to a LAN address");
  Check(NormalizeBaseUrl("http://10.1.2.3") == "http://10.1.2.3", "http to 10.x");
  Check(NormalizeBaseUrl("http://172.16.0.1") == "http://172.16.0.1", "http to 172.16");
  Check(NormalizeBaseUrl("http://172.31.255.1") == "http://172.31.255.1", "http to 172.31");
  Check(NormalizeBaseUrl("http://172.32.0.1").empty(), "172.32 is not private");
  Check(NormalizeBaseUrl("http://172.15.0.1").empty(), "172.15 is not private");
  Check(NormalizeBaseUrl("http://192.169.0.1").empty(), "192.169 is not private");
  Check(NormalizeBaseUrl("ftp://host").empty(), "other schemes are refused");
  Check(NormalizeBaseUrl("gw2.rauwman.org").empty(), "a scheme is required");
  Check(NormalizeBaseUrl("https://").empty(), "a host is required");
  Check(NormalizeBaseUrl("").empty() && NormalizeBaseUrl("   ").empty(), "empty is unusable");
  Check(NormalizeBaseUrl("https://host/a b").empty(), "spaces inside are refused");
  Check(NormalizeBaseUrl("https://host/?x=1").empty(), "a query is refused");
}

static void TestJson() {
  Section("json");
  Check(JsonEscape("a\"b\\c\n\t") == "a\\\"b\\\\c\\n\\t", "escapes quote, backslash and control characters");
  Check(JsonEscape(std::string("\x01")) == "\\u0001", "low control characters become \\u escapes");
  Check(UrlEncode("Yann R") == "Yann%20R", "space is encoded");
  Check(UrlEncode("a-b_c.d~e") == "a-b_c.d~e", "unreserved characters stay");

  auto j = ParseFlatJson(R"({"xuid":"E00014A12C71D845","name":"Yann R","token":"abc-_","ok":true,"n":42,"x":null})");
  Check(j["xuid"] == "E00014A12C71D845" && j["name"] == "Yann R" && j["token"] == "abc-_", "strings");
  Check(j["ok"] == "true" && j["n"] == "42", "booleans and numbers come as text");
  Check(j.count("x") == 0, "null is skipped");

  j = ParseFlatJson(R"({"detail":[{"loc":["body","name"],"msg":"too long"}],"after":"yes"})");
  Check(j.count("detail") == 0 && j["after"] == "yes", "an array value is skipped and parsing goes on");
  j = ParseFlatJson(R"({"a":{"b":"}","c":[1,2,"]"]},"z":"1"})");
  Check(j.count("a") == 0 && j["z"] == "1", "nested braces inside strings do not confuse the skip");
  j = ParseFlatJson(R"({"s":"q\"uote\\ ä\n"})");
  Check(j["s"] == "q\"uote\\ \xC3\xA4\n", "escapes and \\u in strings");
  Check(ParseFlatJson("").empty() && ParseFlatJson("not json").empty() && ParseFlatJson("[1]").empty(),
        "no object, no members");
  Check(ParseFlatJson("{\"a\":\"unterminated").count("a") == 0, "a cut-off string is dropped");
  Check(ParseFlatJson("{\"a\":").empty(), "a missing value ends parsing");
}

static void TestCheckName() {
  Section("name check");
  int calls = 0;
  FakeTransport free_name([&](const HttpRequest&, int) {
    ++calls;
    return Answer(200, R"({"available":true,"reason":null})");
  });
  NameCheck r = CheckName(free_name, "https://s", "Yann R");
  Check(r.state == NameState::kAvailable, "a free name");
  const auto sent = free_name.Requests();
  Check(sent.size() == 1 && sent[0].method == "GET" && sent[0].url == "https://s/v1/name-available?name=Yann%20R" &&
            sent[0].bearer.empty(),
        "GET with the encoded name and no token");

  r = CheckName(free_name, "https://s", "Ünal");
  Check(r.state == NameState::kInvalid && !r.message.empty() && calls == 1, "a bad name is rejected without a request");
  r = CheckName(free_name, "", "Yann");
  Check(r.state == NameState::kUnreachable && calls == 1, "no server address, no request");

  FakeTransport taken([](const HttpRequest&, int) { return Answer(200, R"({"available":false,"reason":"taken"})"); });
  Check(CheckName(taken, "https://s", "Yann").state == NameState::kTaken, "a taken name");
  FakeTransport invalid([](const HttpRequest&, int) {
    return Answer(200, R"({"available":false,"reason":"invalid","message":"Server says no"})");
  });
  r = CheckName(invalid, "https://s", "Yann");
  Check(r.state == NameState::kInvalid && r.message == "Server says no", "the server's reason is passed on");
  FakeTransport limited([](const HttpRequest&, int) { return Answer(429); });
  Check(CheckName(limited, "https://s", "Yann").state == NameState::kUnreachable, "rate limited");
  FakeTransport down([](const HttpRequest&, int) { return Answer(0); });
  Check(CheckName(down, "https://s", "Yann").state == NameState::kUnreachable, "no connection");
  FakeTransport odd([](const HttpRequest&, int) { return Answer(200, "<html>proxy page</html>"); });
  Check(CheckName(odd, "https://s", "Yann").state == NameState::kUnreachable, "an answer that is not the API");
}

static void TestRegister() {
  Section("register");
  FakeTransport ok([](const HttpRequest&, int) {
    return Answer(201, R"({"xuid":"E00014A12C71D845","name":"Yann R","token":"tok-123"})");
  });
  RegisterResult r = Register(ok, "https://s", "Yann R", "se\"cret");
  Check(r.ok() && r.account.token == "tok-123" && r.account.name == "Yann R" && r.account.xuid == "E00014A12C71D845",
        "success returns the account");
  const auto sent = ok.Requests();
  Check(sent.size() == 1 && sent[0].method == "POST" && sent[0].url == "https://s/v1/register" &&
            sent[0].body == R"({"name":"Yann R","invite":"se\"cret"})" && sent[0].bearer.empty(),
        "POST with name and escaped invite code, no token");

  const auto fails = [](int status, std::string body = {}) {
    FakeTransport t([=](const HttpRequest&, int) { return Answer(status, body); });
    return Register(t, "https://s", "Yann", "code");
  };
  Check(fails(403, R"({"detail":"Wrong invite code"})").error == AccountError::kWrongInvite, "wrong invite code");
  Check(fails(403, R"({"detail":"Registration is closed"})").error == AccountError::kClosed, "registration closed");
  Check(fails(409).error == AccountError::kNameTaken, "name taken");
  r = fails(422, R"({"detail":"Start with a letter or a digit."})");
  Check(r.error == AccountError::kInvalidName && r.message == "Start with a letter or a digit.",
        "the server's name rule is shown");
  r = fails(422, R"({"detail":[{"msg":"too long"}]})");
  Check(r.error == AccountError::kInvalidName && !r.message.empty(), "a validation list falls back to a plain sentence");
  Check(fails(429).error == AccountError::kRateLimited, "rate limited");
  Check(fails(0).error == AccountError::kUnreachable, "no connection");
  Check(fails(500).error == AccountError::kServer, "server error");
  Check(fails(201, "{}").error == AccountError::kServer, "success without a token is not trusted");
  Check(!fails(409).message.empty(), "every failure has a message");

  int calls = 0;
  FakeTransport none([&](const HttpRequest&, int) {
    ++calls;
    return Answer(201);
  });
  r = Register(none, "https://s", "Bad/Name", "code");
  Check(r.error == AccountError::kInvalidName && calls == 0, "a bad name never reaches the server");
}

static void TestRename() {
  Section("rename");
  FakeTransport ok([](const HttpRequest&, int) { return Answer(200, R"({"xuid":"E000","name":"New"})"); });
  Check(Rename(ok, "https://s", "tok", "New").ok(), "success");
  const auto sent = ok.Requests();
  Check(sent.size() == 1 && sent[0].method == "PUT" && sent[0].url == "https://s/v1/me" && sent[0].bearer == "tok" &&
            sent[0].body == R"({"name":"New"})",
        "PUT /v1/me with the token");
  const auto code = [](int status) {
    FakeTransport t([=](const HttpRequest&, int) { return Answer(status); });
    return Rename(t, "https://s", "tok", "New").error;
  };
  Check(code(401) == AccountError::kBadToken, "unknown token");
  Check(code(409) == AccountError::kNameTaken, "name taken");
  Check(code(422) == AccountError::kInvalidName, "invalid name");
  Check(code(0) == AccountError::kUnreachable, "no connection");
  Check(code(502) == AccountError::kServer, "bad gateway");
  Check(Rename(ok, "https://s", "tok", "").error == AccountError::kInvalidName, "an empty name is refused locally");
}

static void TestRestore() {
  Section("restore");
  FakeTransport ok([](const HttpRequest&, int) { return Answer(200, R"({"xuid":"E00000000000002A","name":"Yann R"})"); });
  const RegisterResult r = Restore(ok, "https://s", "secret-token");
  Check(r.ok() && r.account.name == "Yann R" && r.account.token == "secret-token" && r.account.xuid == "E00000000000002A",
        "the server's name and id, the same key");
  const auto sent = ok.Requests();
  Check(sent.size() == 1 && sent[0].method == "GET" && sent[0].url == "https://s/v1/me" && sent[0].bearer == "secret-token",
        "GET /v1/me with the key");
  const auto code = [](int status) {
    FakeTransport t([=](const HttpRequest&, int) { return Answer(status); });
    return Restore(t, "https://s", "tok").error;
  };
  Check(code(401) == AccountError::kBadToken, "unknown key");
  Check(code(0) == AccountError::kUnreachable, "no connection");
  Check(code(429) == AccountError::kRateLimited, "rate limited");
  Check(code(500) == AccountError::kServer, "server error");
  FakeTransport odd([](const HttpRequest&, int) { return Answer(200, "<html>proxy</html>"); });
  Check(Restore(odd, "https://s", "tok").error == AccountError::kServer, "a reply without a name is an error");
  int calls = 0;
  FakeTransport counting([&](const HttpRequest&, int) { ++calls; return Answer(200); });
  Check(Restore(counting, "https://s", "").error == AccountError::kBadToken &&
            Restore(counting, "https://s", "two words").error == AccountError::kBadToken &&
            Restore(counting, "https://s", std::string(300, 'a')).error == AccountError::kBadToken && calls == 0,
        "a malformed key never reaches the server");
}

static void TestPostScores() {
  Section("score upload");
  FakeTransport ok([](const HttpRequest&, int) { return Answer(200, R"({"revision":3,"results":[]})"); });
  std::string message;
  Check(PostScores(ok, "https://s", "tok", {{1, 123456}, {4, 9}}, &message) == UploadResult::kOk, "success");
  const auto sent = ok.Requests();
  Check(sent.size() == 1 && sent[0].method == "POST" && sent[0].url == "https://s/v1/scores" && sent[0].bearer == "tok" &&
            sent[0].body == R"({"scores":[{"view":1,"score":123456},{"view":4,"score":9}]})",
        "POST /v1/scores with every view");
  const auto result = [](int status) {
    FakeTransport t([=](const HttpRequest&, int) { return Answer(status); });
    return PostScores(t, "https://s", "tok", {{0, 1}});
  };
  Check(result(401) == UploadResult::kRejected, "an unknown token is final");
  Check(result(422) == UploadResult::kRejected, "a refused request is final");
  Check(result(404) == UploadResult::kRejected, "a wrong address is final");
  Check(result(0) == UploadResult::kRetry, "no connection is retried");
  Check(result(429) == UploadResult::kRetry, "rate limiting is retried");
  Check(result(503) == UploadResult::kRetry, "a failing server is retried");
  FakeTransport down([](const HttpRequest&, int) {
    HttpResponse r;
    r.error = "could not connect to the server";
    return r;
  });
  PostScores(down, "https://s", "tok", {{0, 1}}, &message);
  Check(message == "could not connect to the server", "the transport's reason is kept for the log");
}

static void TestRetryDelay() {
  Section("retry delay");
  Check(RetryDelayMs(0, 15000, 900000) == 15000, "first retry");
  Check(RetryDelayMs(1, 15000, 900000) == 30000 && RetryDelayMs(2, 15000, 900000) == 60000, "doubles");
  Check(RetryDelayMs(10, 15000, 900000) == 900000, "capped");
  Check(RetryDelayMs(1000000, 15000, 900000) == 900000, "no overflow for a huge attempt count");
}

static void TestClient() {
  Section("background upload");
  const ClientConfig config{"https://s", "tok"};
  std::mutex score_mutex;
  std::map<uint32_t, int64_t> scores{{1, 100}};
  const OnlineClient::Snapshot snapshot = [&] {
    std::lock_guard lock(score_mutex);
    return scores;
  };
  std::vector<std::string> logs;
  std::mutex log_mutex;
  const OnlineClient::Log log = [&](const std::string& line) {
    std::lock_guard lock(log_mutex);
    logs.push_back(line);
  };

  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(200); });
    FakeTransport* seen = transport.get();
    OnlineClient client(config, std::move(transport), snapshot, log);
    Check(WaitFor([&] { return client.Uploads() == 1; }), "the first upload happens by itself at start");
    {
      std::lock_guard lock(score_mutex);
      scores[1] = 250;
      scores[3] = 7;
    }
    client.ScoresChanged();
    Check(WaitFor([&] { return client.Uploads() == 2; }), "a changed score is uploaded");
    const auto sent = seen->Requests();
    Check(sent.size() == 2 && sent[0].body == R"({"scores":[{"view":1,"score":100}]})" &&
              sent[1].body == R"({"scores":[{"view":1,"score":250},{"view":3,"score":7}]})",
          "each upload carries the whole current snapshot");
    Check(!client.Rejected(), "not rejected");
  }

  {
    scores = {};
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(200); });
    FakeTransport* seen = transport.get();
    OnlineClient client(config, std::move(transport), snapshot, log);
    client.ScoresChanged();
    std::this_thread::sleep_for(100ms);
    Check(seen->Requests().empty() && client.Uploads() == 0, "nothing to upload, no request");
    scores = {{1, 100}};
  }

  {
    auto transport = std::make_unique<FakeTransport>(
        [](const HttpRequest&, int call) { return call <= 2 ? Answer(503) : Answer(200); });
    FakeTransport* seen = transport.get();
    OnlineClient client(config, std::move(transport), snapshot, log, {10, 40});
    Check(WaitFor([&] { return client.Uploads() == 1; }), "a failing server is retried until it works");
    Check(seen->Requests().size() == 3, "two failures then a success: three requests");
    std::lock_guard lock(log_mutex);
    Check(!logs.empty() && logs.back().find("retry") != std::string::npos, "the first failure is logged once");
  }

  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(401); });
    FakeTransport* seen = transport.get();
    OnlineClient client(config, std::move(transport), snapshot, log);
    Check(WaitFor([&] { return client.Rejected(); }), "a rejected token switches uploads off");
    client.ScoresChanged();
    std::this_thread::sleep_for(100ms);
    Check(seen->Requests().size() == 1, "and nothing more is sent");
  }

  {
    // A long pause after a failure must not hold back a new score.
    auto transport = std::make_unique<FakeTransport>(
        [](const HttpRequest&, int call) { return call == 1 ? Answer(0) : Answer(200); });
    FakeTransport* seen = transport.get();
    OnlineClient client(config, std::move(transport), snapshot, log, {60000, 60000});
    Check(WaitFor([&] { return seen->Requests().size() == 1; }), "first request fails");
    std::this_thread::sleep_for(50ms);
    client.ScoresChanged();
    Check(WaitFor([&] { return client.Uploads() == 1; }), "a new score ends the pause early");
  }

  {
    // Shutting down during a long pause is quick.
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(0); });
    FakeTransport* seen = transport.get();
    const auto start = std::chrono::steady_clock::now();
    {
      OnlineClient client(config, std::move(transport), snapshot, log, {60000, 60000});
      WaitFor([&] { return seen->Requests().size() == 1; });
    }
    Check(std::chrono::steady_clock::now() - start < 2s, "the destructor does not wait out the pause");
  }
}

int main() {
  TestNames();
  TestBaseUrl();
  TestJson();
  TestCheckName();
  TestRegister();
  TestRename();
  TestRestore();
  TestPostScores();
  TestRetryDelay();
  TestClient();
  std::printf("\n%s: %d failure(s)\n", g_failures ? "ONLINE TESTS FAILED" : "ONLINE TESTS OK", g_failures);
  return g_failures ? 1 : 0;
}
