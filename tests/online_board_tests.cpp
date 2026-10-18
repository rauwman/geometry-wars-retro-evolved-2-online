// Tests of the friends' boards: the JSON reader, the server's answer, the merge with the player's own score, the
// windows the game asks for, the background cache and the leaderboard the game's queries are answered from.
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "online_board.h"

using namespace gw2;
using namespace gw2::online;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

static int g_failures = 0;
static void Check(bool ok, const std::string& what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}
static void Section(const char* name) { std::printf("\n== %s\n", name); }

static bool WaitFor(const std::function<bool()>& condition, std::chrono::milliseconds limit = 3000ms) {
  const auto end = std::chrono::steady_clock::now() + limit;
  while (std::chrono::steady_clock::now() < end) {
    if (condition()) return true;
    std::this_thread::sleep_for(5ms);
  }
  return condition();
}

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

 private:
  Handler handler_;
  std::shared_ptr<std::vector<HttpRequest>> log_;
  std::shared_ptr<std::mutex> mutex_;
};

static HttpResponse Answer(int status, std::string body = {}, std::string etag = {}) {
  HttpResponse r;
  r.status = status;
  r.body = std::move(body);
  r.etag = std::move(etag);
  return r;
}

static const char kBoards[] = R"({
  "revision": 7, "me": "E000000000000002",
  "views": {
    "0": {"total": 3, "rows": [
      {"rank": 1, "xuid": "E000000000000001", "name": "Anna", "score": 900},
      {"rank": 2, "xuid": "E000000000000002", "name": "Yann R", "score": 500},
      {"rank": 3, "xuid": "E000000000000003", "name": "Ben ä", "score": 100}]},
    "1": {"total": 0, "rows": []},
    "9": {"total": 1, "rows": [{"rank": 1, "xuid": "E000000000000001", "name": "Anna", "score": 5}]}
  }})";

static void TestJson() {
  Section("json");
  auto v = ParseJson(R"({"a":[1,-2,{"b":"x\ny"}],"t":true,"n":null,"s":"q\"\\"})");
  Check(v && v->type == JsonValue::Type::kObject, "an object");
  Check(v && v->Find("a")->items.size() == 3 && v->Find("a")->items[1].number == -2, "arrays and negative numbers");
  Check(v && v->Find("a")->items[2].Find("b")->text == "x\ny", "nested strings with escapes");
  Check(v && v->Find("t")->boolean && v->Find("n")->type == JsonValue::Type::kNull, "literals");
  Check(v && v->Find("s")->text == "q\"\\", "escaped quote and backslash");
  Check(!v->Find("missing"), "a missing key is a null pointer");
  v = ParseJson(R"({"u":"ä€"})");
  Check(v && v->Find("u")->text == "\xC3\xA4\xE2\x82\xAC", "\\u escapes become UTF-8");
  v = ParseJson(R"({"x": 1.5e3})");
  Check(v && v->Find("x")->number == 1, "a fraction and exponent are read past");
  Check(!ParseJson("") && !ParseJson("{") && !ParseJson("[1,]") && !ParseJson("{\"a\":}") && !ParseJson("{} x"),
        "malformed input is refused");
  Check(!ParseJson(R"({"a":"unterminated)") && !ParseJson(R"({"a":"\q"})"), "bad strings are refused");
  Check(!ParseJson(std::string(20, '[') + std::string(20, ']')), "nesting is limited");
  Check(!ParseJson("{\"n\":99999999999999999999}"), "an overflowing number is refused");
}

static void TestParseBoards() {
  Section("the server's boards");
  const auto b = ParseBoards(kBoards);
  Check(b && b->revision == 7 && b->me == "E000000000000002", "revision and the player's own id");
  Check(b && b->views.size() == 3, "every view with an id the game knows");
  Check(b && b->views.at(0).rows.size() == 3 && b->views.at(0).total == 3 && b->views.at(0).rows[0].name == "Anna" &&
            b->views.at(0).rows[2].name == "Ben \xC3\xA4",
        "rows in order with their names");
  Check(b && b->views.at(1).rows.empty(), "an empty view");
  Check(!ParseBoards("[]") && !ParseBoards("{}") && !ParseBoards("nonsense"), "not a leaderboard");
  Check(!ParseBoards(R"({"views":{"0":{"total":1,"rows":[{"xuid":"E1","name":"A"}]}}})"), "a row without a score");
  Check(!ParseBoards(R"({"views":{"0":{"total":1,"rows":[{"xuid":"E1","name":"A","score":0}]}}})"),
        "a row without a positive score");
  const auto odd = ParseBoards(R"({"views":{"x":{"total":0,"rows":[]},"99":{"total":0,"rows":[]},"2":{"total":0,"rows":[]}}})");
  Check(odd && odd->views.size() == 1 && odd->views.count(2) == 1, "ids that are not views are skipped");
  Check(XuidFromHex("E00000000002") == 0 && XuidFromHex("E00000000000000A") == 0xE00000000000000Aull &&
            XuidFromHex("zzzzzzzzzzzzzzzz") == 0,
        "ids are 16 hex digits");
}

static void TestCombined() {
  Section("merge with the player's own score");
  const auto b = ParseBoards(kBoards);
  OwnPlayer own{"E000000000000002", 0xB13EBABEBABEBABEull, "Yann R", std::nullopt};

  auto all = CombinedView(0, &b->views.at(0), own);
  Check(all.rows.size() == 3 && all.total_rows == 3 && all.rows[0].rank == 1 && all.rows[2].rank == 3,
        "the server's rows keep their ranks");
  Check(all.rows[1].xuid == own.local_xuid && all.rows[1].gamertag == "Yann R",
        "the player's own row carries the game's own id");
  Check(all.rows[0].xuid == 0xE000000000000001ull && all.rows[0].rating == 900 && all.rows[0].gamertag == "Anna",
        "others carry the server's id");

  own.best = 1500;
  all = CombinedView(0, &b->views.at(0), own);
  Check(all.rows[0].gamertag == "Yann R" && all.rows[0].rating == 1500 && all.rows[1].gamertag == "Anna" &&
            all.rows.size() == 3,
        "a better score on this PC moves the player up without a duplicate row");
  own.best = 50;
  all = CombinedView(0, &b->views.at(0), own);
  Check(all.rows[1].gamertag == "Yann R" && all.rows[1].rating == 500, "a worse local score changes nothing");

  // The player is not on the server's board for this view yet.
  own.best = 400;
  all = CombinedView(1, &b->views.at(1), own);
  Check(all.rows.size() == 1 && all.rows[0].gamertag == "Yann R" && all.rows[0].rank == 1 && all.total_rows == 1,
        "a score the server does not know yet is shown");
  own.best = 100;
  all = CombinedView(0, &ParseBoards(kBoards)->views.at(0), OwnPlayer{"", own.local_xuid, "Yann R", 100});
  Check(all.rows.size() == 4 && all.rows[3].gamertag == "Yann R" && all.rows[2].gamertag == "Ben \xC3\xA4",
        "a tie is ranked behind the earlier score");
  all = CombinedView(3, nullptr, OwnPlayer{"E1", own.local_xuid, "Yann R", std::nullopt});
  Check(all.rows.empty() && all.total_rows == 0, "no board and no score: nothing");
}

static live::StatsView Ten() {
  live::StatsView v;
  v.id = 0;
  v.total_rows = 10;
  for (uint32_t i = 1; i <= 10; ++i) v.rows.push_back({i * 100ull, i, 1000 - int64_t(i), "P" + std::to_string(i)});
  return v;
}

static void TestWindows() {
  Section("windows");
  const live::StatsView all = Ten();
  auto w = RankedWindow(all, 1, 3);
  Check(w.rows.size() == 3 && w.rows[0].rank == 1 && w.rows[2].rank == 3 && w.total_rows == 10, "the first rows");
  w = RankedWindow(all, 4, 3);
  Check(w.rows.size() == 3 && w.rows[0].rank == 4, "from a later rank");
  w = RankedWindow(all, 9, 5);
  Check(w.rows.size() == 2 && w.rows[1].rank == 10, "the end of the board");
  w = RankedWindow(all, 0, 2);
  Check(w.rows.size() == 2 && w.rows[0].rank == 1, "rank 0 means the first");
  w = RankedWindow(all, 20, 5);
  Check(w.rows.empty() && w.total_rows == 10, "past the end");
  Check(RankedWindow(all, 1, 0).rows.empty(), "no rows asked");

  w = AroundWindow(all, 500, 5);
  Check(w.rows.size() == 5 && w.rows[0].rank == 3 && w.rows[2].rank == 5 && w.rows[4].rank == 7, "own row in the middle");
  w = AroundWindow(all, 100, 5);
  Check(w.rows.size() == 5 && w.rows[0].rank == 1, "near the top it starts at rank 1");
  w = AroundWindow(all, 1000, 5);
  Check(w.rows.size() == 3 && w.rows[0].rank == 8 && w.rows[2].rank == 10, "near the bottom it ends at the last rank");
  Check(AroundWindow(all, 12345, 5).rows.empty(), "a player without a row has no window");
  Check(AroundWindow(all, 500, 0).rows.empty(), "no rows asked");
  w = AroundWindow(all, 500, 1);
  Check(w.rows.size() == 1 && w.rows[0].rank == 5, "one row is the player's own");
}

static void TestFetch() {
  Section("fetching");
  auto sent = std::make_shared<std::vector<HttpRequest>>();
  FakeTransport ok([](const HttpRequest&, int) { return Answer(200, kBoards, "\"7-100\""); }, sent);
  FetchResult r = FetchBoards(ok, "https://s", "tok", "");
  Check(r.kind == FetchKind::kUpdated && r.snapshot && r.snapshot->revision == 7 && r.etag == "\"7-100\"", "an update");
  Check(sent->size() == 1 && (*sent)[0].method == "GET" && (*sent)[0].url == "https://s/v1/boards?limit=100" &&
            (*sent)[0].bearer == "tok" && (*sent)[0].if_none_match.empty(),
        "GET with the token and the row limit");
  FakeTransport same([](const HttpRequest&, int) { return Answer(304); });
  r = FetchBoards(same, "https://s", "tok", "\"7-100\"");
  Check(r.kind == FetchKind::kUnchanged && r.etag == "\"7-100\"" && !r.snapshot, "unchanged keeps the ETag");
  FakeTransport gone([](const HttpRequest&, int) { return Answer(401); });
  Check(FetchBoards(gone, "https://s", "tok", "").kind == FetchKind::kRejected, "a refused token");
  FakeTransport busy([](const HttpRequest&, int) { return Answer(503); });
  r = FetchBoards(busy, "https://s", "tok", "");
  Check(r.kind == FetchKind::kFailed && !r.message.empty(), "a failing server");
  FakeTransport down([](const HttpRequest&, int) { return Answer(0); });
  Check(FetchBoards(down, "https://s", "tok", "").kind == FetchKind::kFailed, "no connection");
  FakeTransport odd([](const HttpRequest&, int) { return Answer(200, "<html>"); });
  Check(FetchBoards(odd, "https://s", "tok", "").kind == FetchKind::kFailed, "an answer that is not the API");
}

static void TestCache() {
  Section("cache");
  const ClientConfig config{"https://s", "tok"};
  std::vector<std::string> logs;
  std::mutex log_mutex;
  const BoardCache::Log log = [&](const std::string& line) {
    std::lock_guard lock(log_mutex);
    logs.push_back(line);
  };
  {
    auto sent = std::make_shared<std::vector<HttpRequest>>();
    auto mutex = std::make_shared<std::mutex>();
    auto transport = std::make_unique<FakeTransport>(
        [](const HttpRequest& request, int) {
          return request.if_none_match == "\"7-100\"" ? Answer(304) : Answer(200, kBoards, "\"7-100\"");
        },
        sent, mutex);
    BoardCache cache(config, std::move(transport), log, {60000});
    Check(WaitFor([&] { return cache.Snapshot() != nullptr; }), "the boards are fetched at start");
    cache.Refresh();
    Check(WaitFor([&] { return cache.Fetches() >= 2; }), "Refresh asks again");
    std::lock_guard lock(*mutex);
    Check(sent->size() >= 2 && (*sent)[1].if_none_match == "\"7-100\"", "the second request carries the ETag");
    Check(cache.Snapshot() && cache.Snapshot()->views.count(0) == 1, "the snapshot survives a 304");
  }
  {
    auto transport = std::make_unique<FakeTransport>(
        [](const HttpRequest&, int call) { return call == 1 ? Answer(0) : Answer(200, kBoards, "\"x\""); });
    BoardCache cache(config, std::move(transport), log, {20});
    Check(WaitFor([&] { return cache.Snapshot() != nullptr; }), "a failed fetch is retried at the poll interval");
    Check(cache.Snapshot() != nullptr && !cache.Rejected(), "and the cache is usable afterwards");
  }
  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(401); });
    BoardCache cache(config, std::move(transport), log, {20});
    Check(WaitFor([&] { return cache.Rejected(); }), "a refused token switches the cache off");
    Check(cache.Snapshot() == nullptr, "and no boards are served");
  }
  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(0); });
    const auto start = std::chrono::steady_clock::now();
    { BoardCache cache(config, std::move(transport), log, {60000}); std::this_thread::sleep_for(50ms); }
    Check(std::chrono::steady_clock::now() - start < 2s, "the destructor does not wait out the poll interval");
  }
}

static void TestLeaderboard() {
  Section("the game's queries");
  const fs::path dir = fs::temp_directory_path() / "gw2_board_tests";
  fs::remove_all(dir);
  LocalStats stats(dir / "stats.toml");
  stats.Load();
  const uint64_t kLocal = 0xB13EBABEBABEBABEull;
  stats.Record(kLocal, 0, 400);
  live::LocalLeaderboard local(stats, {kLocal, "User"});

  const ClientConfig config{"https://s", "tok"};
  auto transport = std::make_unique<FakeTransport>(
      [](const HttpRequest&, int) { return Answer(200, kBoards, "\"7-100\""); });
  BoardCache cache(config, std::move(transport), nullptr, {60000});
  RemoteLeaderboard board(local, cache, "Yann R", [&](uint32_t view) { return stats.Best(kLocal, view); });

  // Before the first answer the offline board speaks.
  if (!cache.Snapshot()) {
    const live::StatsView early = board.Ranked(0, 1, 10);
    Check(early.rows.size() <= 1, "before the boards arrive, only the player's own row");
  }
  Check(WaitFor([&] { return cache.Snapshot() != nullptr; }), "boards arrived");
  Check(cache.WaitForFirstAnswer(10), "an answered cache does not make a caller wait");

  live::StatsView v = board.Ranked(0, 1, 10);
  Check(v.rows.size() == 3 && v.rows[0].gamertag == "Anna" && v.rows[1].gamertag == "Yann R" && v.rows[1].rating == 500 &&
            v.rows[1].xuid == kLocal && v.total_rows == 3,
        "the friends' board, with the player's own row under the game's id");
  stats.Record(kLocal, 0, 2000);
  v = board.Ranked(0, 1, 10);
  Check(v.rows[0].gamertag == "Yann R" && v.rows[0].rating == 2000, "a new local best shows at once, before the upload");
  v = board.Around(0, kLocal, 3);
  Check(v.rows.size() == 3 && v.rows[0].rank == 1, "around the player");
  v = board.Around(0, 12345, 3);
  Check(v.rows.empty(), "around someone else: nothing");
  v = board.Ranked(2, 1, 10);
  Check(v.rows.empty() && v.id == 2, "a view nobody has scored in");
  v = board.Selected(0, {kLocal});
  Check(v.rows.size() == 1 && v.rows[0].rating == 2000 && v.rows[0].xuid == kLocal,
        "the player's own stats: one row, local id");
  const auto friends = board.Friends();
  Check(friends.size() == 2 && friends[0].name == "Anna", "the other server players are the friends");
  v = board.Selected(0, {kLocal, friends[0].xuid});
  Check(v.rows.size() == 2 && v.rows[0].xuid == kLocal && v.rows[0].rank == 1 && v.rows[1].gamertag == "Anna" && v.rows[1].rank == 2,
        "a read for the player and a friend returns both, ranked as on the board");
  v = board.Selected(2, {kLocal});
  Check(v.rows.size() == 1 && v.rows[0].xuid == kLocal, "a view nobody played still gets the player's own row");
  v = board.Selected(0, {12345});
  Check(v.rows.empty(), "a stranger has no row");
  fs::remove_all(dir);
}

static void TestEarlyFriends() {
  Section("friends asked for at startup");
  const fs::path dir = fs::temp_directory_path() / "gw2_board_early_tests";
  fs::remove_all(dir);
  LocalStats stats(dir / "stats.toml");
  stats.Load();
  const uint64_t kLocal = 0xB13EBABEBABEBABEull;
  live::LocalLeaderboard local(stats, {kLocal, "User"});
  const ClientConfig config{"https://s", "tok"};

  // The server answers a little late: the question waits for it and gets the players.
  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) {
      std::this_thread::sleep_for(150ms);
      return Answer(200, kBoards, "\"7-100\"");
    });
    BoardCache cache(config, std::move(transport), nullptr, {60000});
    RemoteLeaderboard board(local, cache, "Yann R", [](uint32_t) { return std::optional<int64_t>(); });
    const auto start = std::chrono::steady_clock::now();
    const auto friends = board.Friends();
    Check(friends.size() == 2, "a slow first answer still gives the friends");
    Check(std::chrono::steady_clock::now() - start < 2000ms, "and does not wait the whole limit");
  }
  // The server is down: an answer (a failure) ends the wait at once and there are no friends.
  {
    auto transport = std::make_unique<FakeTransport>([](const HttpRequest&, int) { return Answer(0); });
    BoardCache cache(config, std::move(transport), nullptr, {60000});
    RemoteLeaderboard board(local, cache, "Yann R", [](uint32_t) { return std::optional<int64_t>(); });
    const auto start = std::chrono::steady_clock::now();
    Check(board.Friends().empty(), "no connection: no friends");
    Check(std::chrono::steady_clock::now() - start < 2000ms, "a failed answer ends the wait early");
  }
  fs::remove_all(dir);
}

int main() {
  TestJson();
  TestParseBoards();
  TestCombined();
  TestWindows();
  TestFetch();
  TestCache();
  TestLeaderboard();
  TestEarlyFriends();
  std::printf("\n%s: %d failure(s)\n", g_failures ? "ONLINE BOARD TESTS FAILED" : "ONLINE BOARD TESTS OK", g_failures);
  return g_failures ? 1 : 0;
}
