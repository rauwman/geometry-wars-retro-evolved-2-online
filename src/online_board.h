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
#include <utility>
#include <vector>

#include "live_stats.h"
#include "online_client.h"

// The friends' boards as the game shows them. Nothing here touches the runtime, so tests\online_board_tests.cpp covers
// all of it with a fake transport.
namespace gw2::online {

// A small JSON value, enough for the server's answers. Nesting is limited, numbers are integers.
struct JsonValue {
  enum class Type { kNull, kBool, kNumber, kString, kArray, kObject };
  Type type = Type::kNull;
  bool boolean = false;
  int64_t number = 0;
  std::string text;
  std::vector<JsonValue> items;
  std::vector<std::pair<std::string, JsonValue>> members;

  const JsonValue* Find(std::string_view key) const;
};

std::optional<JsonValue> ParseJson(std::string_view text);

struct BoardRow {
  std::string xuid;  // the server's id for the player, 16 hex digits
  std::string name;
  int64_t score = 0;
};

struct ServerBoard {
  uint32_t total = 0;
  std::vector<BoardRow> rows;  // best first
};

struct BoardsSnapshot {
  int64_t revision = 0;
  std::string me;  // the server's id for the signed-in player
  std::map<uint32_t, ServerBoard> views;
};

// The answer of GET /v1/boards. Nothing when it is not that.
std::optional<BoardsSnapshot> ParseBoards(std::string_view json);

// The server's 16 hex digits as the game's 64-bit id; 0 when it is not that.
uint64_t XuidFromHex(std::string_view hex);

// The signed-in player as the game knows them.
struct OwnPlayer {
  std::string server_xuid;  // as in BoardsSnapshot::me
  uint64_t local_xuid = 0;  // the id the game's own profile has
  std::string name;
  std::optional<int64_t> best;  // the clean score on this PC in this view, which the server may not have yet
};

// Every row of one view, ranked from 1: the server's rows, with the player's own row under the game's own id and
// carrying the better of the server's score and the one on this PC.
live::StatsView CombinedView(uint32_t view, const ServerBoard* board, const OwnPlayer& own);

// `max_rows` rows from `first_rank` on.
live::StatsView RankedWindow(const live::StatsView& all, uint32_t first_rank, uint32_t max_rows);
// `max_rows` rows with the player's own row in the middle. Empty when the player has no row.
live::StatsView AroundWindow(const live::StatsView& all, uint64_t local_xuid, uint32_t max_rows);

enum class FetchKind { kUpdated, kUnchanged, kRejected, kFailed };

struct FetchResult {
  FetchKind kind = FetchKind::kFailed;
  std::shared_ptr<const BoardsSnapshot> snapshot;  // set for kUpdated
  std::string etag;
  std::string message;
};

FetchResult FetchBoards(HttpTransport& transport, const std::string& base_url, const std::string& token,
                        const std::string& etag);

struct BoardTiming {
  int poll_ms = 30000;
};

// Keeps the latest boards, fetched in the background: at start, on Refresh() and every poll interval. The game's
// threads only call Snapshot(), which never waits for the network.
class BoardCache {
 public:
  using Log = std::function<void(const std::string&)>;

  BoardCache(ClientConfig config, std::unique_ptr<HttpTransport> transport, Log log, BoardTiming timing = {});
  ~BoardCache();
  BoardCache(const BoardCache&) = delete;
  BoardCache& operator=(const BoardCache&) = delete;

  void Refresh();
  std::shared_ptr<const BoardsSnapshot> Snapshot() const;
  // Waits until the server has answered once (boards, "unchanged", a refusal or a failure) or `timeout_ms` passed.
  // For the one early question that cannot be answered later; never for a screen that can wait.
  bool WaitForFirstAnswer(int timeout_ms) const;

  // For tests and diagnostics.
  int Fetches() const;
  bool Rejected() const;

 private:
  void Run();

  ClientConfig config_;
  std::unique_ptr<HttpTransport> transport_;
  Log log_;
  BoardTiming timing_;

  mutable std::mutex mutex_;
  mutable std::condition_variable answered_;
  int attempts_ = 0;  // finished fetches, whatever their result
  std::condition_variable wake_;
  bool stop_ = false;
  bool wanted_ = true;
  bool rejected_ = false;
  int fetches_ = 0;
  std::string etag_;
  std::shared_ptr<const BoardsSnapshot> snapshot_;
  std::thread worker_;
};

// The game's leaderboard queries, answered from the friends' boards. Without boards (not fetched yet, no
// connection) the player's own offline board answers, as before.
class RemoteLeaderboard final : public live::BoardSource {
 public:
  using OwnBest = std::function<std::optional<int64_t>(uint32_t view)>;

  RemoteLeaderboard(const live::LocalLeaderboard& local, const BoardCache& cache, std::string name, OwnBest own_best)
      : local_(local), cache_(cache), name_(std::move(name)), own_best_(std::move(own_best)) {}

  const live::LocalUser& user() const override { return local_.user(); }
  live::StatsView Ranked(uint32_t view, uint32_t first_rank, uint32_t max_rows) const override;
  live::StatsView Around(uint32_t view, uint64_t xuid, uint32_t max_rows) const override;
  live::StatsView Selected(uint32_t view, const std::vector<uint64_t>& xuids) const override;
  std::vector<live::FriendInfo> Friends() const override;

 private:
  live::StatsView Combined(uint32_t view, const BoardsSnapshot& boards) const;

  const live::LocalLeaderboard& local_;
  const BoardCache& cache_;
  std::string name_;
  OwnBest own_best_;
};

}  // namespace gw2::online
