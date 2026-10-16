#include "online_board.h"

#include <algorithm>
#include <cctype>
#include <chrono>

namespace gw2::online {
namespace {

constexpr int kMaxDepth = 8;
constexpr uint32_t kBoardLimit = 100;

class JsonReader {
 public:
  explicit JsonReader(std::string_view text) : text_(text) {}

  std::optional<JsonValue> Parse() {
    JsonValue value;
    if (!Value(value, 0)) return std::nullopt;
    Skip();
    if (pos_ != text_.size()) return std::nullopt;
    return value;
  }

 private:
  void Skip() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) ++pos_;
  }

  bool Literal(std::string_view word) {
    if (text_.substr(pos_, word.size()) != word) return false;
    pos_ += word.size();
    return true;
  }

  bool String(std::string& out) {
    if (pos_ >= text_.size() || text_[pos_] != '"') return false;
    ++pos_;
    while (pos_ < text_.size()) {
      const char c = text_[pos_++];
      if (c == '"') return true;
      if (c != '\\') {
        out += c;
        continue;
      }
      if (pos_ >= text_.size()) return false;
      const char escape = text_[pos_++];
      switch (escape) {
        case '"': case '\\': case '/': out += escape; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          if (pos_ + 4 > text_.size()) return false;
          unsigned code = 0;
          for (int i = 0; i < 4; ++i) {
            const char h = text_[pos_++];
            code <<= 4;
            if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
            else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
            else return false;
          }
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
          break;
        }
        default: return false;
      }
    }
    return false;
  }

  bool Number(JsonValue& out) {
    const size_t start = pos_;
    if (pos_ < text_.size() && text_[pos_] == '-') ++pos_;
    const size_t digits = pos_;
    int64_t value = 0;
    while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
      const int digit = text_[pos_++] - '0';
      if (value > (INT64_MAX - digit) / 10) return false;
      value = value * 10 + digit;
    }
    if (pos_ == digits) return false;
    // A fraction or exponent is read past and ignored: the server sends integers.
    if (pos_ < text_.size() && text_[pos_] == '.') {
      ++pos_;
      while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
      ++pos_;
      if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
      while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
    }
    out.type = JsonValue::Type::kNumber;
    out.number = text_[start] == '-' ? -value : value;
    return true;
  }

  bool Value(JsonValue& out, int depth) {
    if (depth > kMaxDepth) return false;
    Skip();
    if (pos_ >= text_.size()) return false;
    const char c = text_[pos_];
    if (c == '{') {
      ++pos_;
      out.type = JsonValue::Type::kObject;
      Skip();
      if (pos_ < text_.size() && text_[pos_] == '}') {
        ++pos_;
        return true;
      }
      for (;;) {
        Skip();
        std::string key;
        if (!String(key)) return false;
        Skip();
        if (pos_ >= text_.size() || text_[pos_++] != ':') return false;
        JsonValue member;
        if (!Value(member, depth + 1)) return false;
        out.members.emplace_back(std::move(key), std::move(member));
        Skip();
        if (pos_ >= text_.size()) return false;
        const char next = text_[pos_++];
        if (next == '}') return true;
        if (next != ',') return false;
      }
    }
    if (c == '[') {
      ++pos_;
      out.type = JsonValue::Type::kArray;
      Skip();
      if (pos_ < text_.size() && text_[pos_] == ']') {
        ++pos_;
        return true;
      }
      for (;;) {
        JsonValue item;
        if (!Value(item, depth + 1)) return false;
        out.items.push_back(std::move(item));
        Skip();
        if (pos_ >= text_.size()) return false;
        const char next = text_[pos_++];
        if (next == ']') return true;
        if (next != ',') return false;
      }
    }
    if (c == '"') {
      out.type = JsonValue::Type::kString;
      return String(out.text);
    }
    if (c == 't') {
      out.type = JsonValue::Type::kBool;
      out.boolean = true;
      return Literal("true");
    }
    if (c == 'f') {
      out.type = JsonValue::Type::kBool;
      return Literal("false");
    }
    if (c == 'n') return Literal("null");
    return Number(out);
  }

  std::string_view text_;
  size_t pos_ = 0;
};

}  // namespace

const JsonValue* JsonValue::Find(std::string_view key) const {
  for (const auto& [name, value] : members) {
    if (name == key) return &value;
  }
  return nullptr;
}

std::optional<JsonValue> ParseJson(std::string_view text) {
  return JsonReader(text).Parse();
}

std::optional<BoardsSnapshot> ParseBoards(std::string_view json) {
  const auto root = ParseJson(json);
  if (!root || root->type != JsonValue::Type::kObject) return std::nullopt;
  const JsonValue* views = root->Find("views");
  if (!views || views->type != JsonValue::Type::kObject) return std::nullopt;

  BoardsSnapshot out;
  if (const JsonValue* revision = root->Find("revision"); revision && revision->type == JsonValue::Type::kNumber) {
    out.revision = revision->number;
  }
  if (const JsonValue* me = root->Find("me"); me && me->type == JsonValue::Type::kString) out.me = me->text;

  for (const auto& [key, board] : views->members) {
    if (board.type != JsonValue::Type::kObject || key.empty() || key.size() > 3) continue;
    uint32_t id = 0;
    bool numeric = true;
    for (const char c : key) {
      if (!std::isdigit(static_cast<unsigned char>(c))) numeric = false;
      id = id * 10 + static_cast<uint32_t>(c - '0');
    }
    if (!numeric || id >= LocalStats::kMaxViews) continue;

    ServerBoard parsed;
    if (const JsonValue* total = board.Find("total"); total && total->type == JsonValue::Type::kNumber && total->number >= 0) {
      parsed.total = static_cast<uint32_t>(std::min<int64_t>(total->number, UINT32_MAX));
    }
    const JsonValue* rows = board.Find("rows");
    if (!rows || rows->type != JsonValue::Type::kArray) return std::nullopt;
    for (const JsonValue& row : rows->items) {
      const JsonValue* xuid = row.Find("xuid");
      const JsonValue* name = row.Find("name");
      const JsonValue* score = row.Find("score");
      if (row.type != JsonValue::Type::kObject || !xuid || !name || !score || xuid->type != JsonValue::Type::kString ||
          name->type != JsonValue::Type::kString || score->type != JsonValue::Type::kNumber || score->number <= 0) {
        return std::nullopt;
      }
      parsed.rows.push_back({xuid->text, name->text, score->number});
    }
    out.views[id] = std::move(parsed);
  }
  return out;
}

uint64_t XuidFromHex(std::string_view hex) {
  if (hex.size() != 16) return 0;
  uint64_t value = 0;
  for (const char c : hex) {
    value <<= 4;
    if (c >= '0' && c <= '9') value |= static_cast<uint64_t>(c - '0');
    else if (c >= 'a' && c <= 'f') value |= static_cast<uint64_t>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F') value |= static_cast<uint64_t>(c - 'A' + 10);
    else return 0;
  }
  return value;
}

live::StatsView CombinedView(uint32_t view, const ServerBoard* board, const OwnPlayer& own) {
  struct Entry {
    uint64_t xuid;
    std::string name;
    int64_t score;
  };
  std::vector<Entry> entries;
  bool own_listed = false;
  uint32_t total = 0;
  if (board) {
    total = board->total;
    for (const BoardRow& row : board->rows) {
      if (!own.server_xuid.empty() && row.xuid == own.server_xuid) {
        own_listed = true;
        const int64_t score = own.best ? std::max(*own.best, row.score) : row.score;
        entries.push_back({own.local_xuid, own.name.empty() ? row.name : own.name, score});
      } else {
        entries.push_back({XuidFromHex(row.xuid), row.name, row.score});
      }
    }
  }
  if (!own_listed && own.best && *own.best > 0) {
    entries.push_back({own.local_xuid, own.name, *own.best});
    ++total;
  }
  std::stable_sort(entries.begin(), entries.end(),
                   [](const Entry& a, const Entry& b) { return a.score > b.score; });

  live::StatsView out;
  out.id = view;
  out.total_rows = std::max<uint32_t>(total, static_cast<uint32_t>(entries.size()));
  uint32_t rank = 1;
  for (Entry& entry : entries) {
    out.rows.push_back({entry.xuid, rank++, entry.score, std::move(entry.name)});
  }
  return out;
}

live::StatsView RankedWindow(const live::StatsView& all, uint32_t first_rank, uint32_t max_rows) {
  live::StatsView out;
  out.id = all.id;
  out.total_rows = all.total_rows;
  const uint32_t first = std::max<uint32_t>(first_rank, 1);
  max_rows = std::min(max_rows, live::kMaxEnumeratedRows);
  for (const live::StatsRow& row : all.rows) {
    if (row.rank < first) continue;
    if (out.rows.size() >= max_rows) break;
    out.rows.push_back(row);
  }
  return out;
}

live::StatsView AroundWindow(const live::StatsView& all, uint64_t local_xuid, uint32_t max_rows) {
  live::StatsView out;
  out.id = all.id;
  out.total_rows = all.total_rows;
  max_rows = std::min(max_rows, live::kMaxEnumeratedRows);
  if (max_rows == 0) return out;
  uint32_t own_rank = 0;
  for (const live::StatsRow& row : all.rows) {
    if (row.xuid == local_xuid) own_rank = row.rank;
  }
  if (!own_rank) return out;
  const uint32_t before = max_rows / 2;
  const uint32_t first = own_rank > before ? own_rank - before : 1;
  return RankedWindow(all, first, max_rows);
}

FetchResult FetchBoards(HttpTransport& transport, const std::string& base_url, const std::string& token,
                        const std::string& etag) {
  HttpRequest request;
  request.method = "GET";
  request.url = base_url + "/v1/boards?limit=" + std::to_string(kBoardLimit);
  request.bearer = token;
  request.if_none_match = etag;
  const HttpResponse response = transport.Send(request);

  FetchResult result;
  result.etag = etag;
  if (response.status == 304) {
    result.kind = FetchKind::kUnchanged;
  } else if (response.status == 401 || response.status == 403) {
    result.kind = FetchKind::kRejected;
    result.message = "the server refused the account";
  } else if (response.status != 200) {
    result.message = response.status == 0 ? (response.error.empty() ? "no connection" : response.error)
                                          : "server answered " + std::to_string(response.status);
  } else if (auto parsed = ParseBoards(response.body)) {
    result.kind = FetchKind::kUpdated;
    result.snapshot = std::make_shared<const BoardsSnapshot>(std::move(*parsed));
    result.etag = response.etag;
  } else {
    result.message = "the answer was not a leaderboard";
  }
  return result;
}

BoardCache::BoardCache(ClientConfig config, std::unique_ptr<HttpTransport> transport, Log log, BoardTiming timing)
    : config_(std::move(config)), transport_(std::move(transport)), log_(std::move(log)), timing_(timing) {
  worker_ = std::thread([this] { Run(); });
}

BoardCache::~BoardCache() {
  {
    std::lock_guard lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  if (worker_.joinable()) worker_.join();
}

void BoardCache::Refresh() {
  {
    std::lock_guard lock(mutex_);
    wanted_ = true;
  }
  wake_.notify_all();
}

std::shared_ptr<const BoardsSnapshot> BoardCache::Snapshot() const {
  std::lock_guard lock(mutex_);
  return snapshot_;
}

int BoardCache::Fetches() const {
  std::lock_guard lock(mutex_);
  return fetches_;
}

bool BoardCache::Rejected() const {
  std::lock_guard lock(mutex_);
  return rejected_;
}

void BoardCache::Run() {
  std::unique_lock lock(mutex_);
  bool failed_before = false;
  while (!stop_) {
    wake_.wait(lock, [this] { return stop_ || wanted_; });
    if (stop_) break;
    wanted_ = false;
    const std::string etag = etag_;
    lock.unlock();

    const FetchResult result = FetchBoards(*transport_, config_.url, config_.token, etag);

    lock.lock();
    switch (result.kind) {
      case FetchKind::kUpdated:
        snapshot_ = result.snapshot;
        etag_ = result.etag;
        ++fetches_;
        failed_before = false;
        break;
      case FetchKind::kUnchanged:
        ++fetches_;
        failed_before = false;
        break;
      case FetchKind::kRejected:
        rejected_ = true;
        if (log_) log_("Friends' boards are off for this session: " + result.message);
        return;
      case FetchKind::kFailed:
        if (!failed_before && log_) log_("Could not load the friends' boards (" + result.message + "), will retry");
        failed_before = true;
        break;
    }
    // The next fetch comes after the poll interval, or sooner when Refresh() is called.
    wake_.wait_for(lock, std::chrono::milliseconds(timing_.poll_ms), [this] { return stop_ || wanted_; });
    wanted_ = true;
  }
}

live::StatsView RemoteLeaderboard::Combined(uint32_t view, const BoardsSnapshot& boards) const {
  OwnPlayer own;
  own.server_xuid = boards.me;
  own.local_xuid = local_.user().xuid;
  own.name = name_;
  if (own_best_) own.best = own_best_(view);
  const auto it = boards.views.find(view);
  return CombinedView(view, it == boards.views.end() ? nullptr : &it->second, own);
}

live::StatsView RemoteLeaderboard::Ranked(uint32_t view, uint32_t first_rank, uint32_t max_rows) const {
  const auto boards = cache_.Snapshot();
  if (!boards) return local_.Ranked(view, first_rank, max_rows);
  return RankedWindow(Combined(view, *boards), first_rank, max_rows);
}

live::StatsView RemoteLeaderboard::Around(uint32_t view, uint64_t xuid, uint32_t max_rows) const {
  const auto boards = cache_.Snapshot();
  if (!boards) return local_.Around(view, xuid, max_rows);
  if (xuid != local_.user().xuid) {
    live::StatsView empty;
    empty.id = view;
    return empty;
  }
  return AroundWindow(Combined(view, *boards), xuid, max_rows);
}

}  // namespace gw2::online
