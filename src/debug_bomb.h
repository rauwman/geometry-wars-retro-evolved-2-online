#pragma once

#include <array>
#include <cstdint>
#include <mutex>

namespace gw2::debug_bomb {

inline constexpr uint32_t kMaxPlayers = 4;
// A press only counts while a player was updated this recently, so F9 outside gameplay does nothing.
inline constexpr double kActiveSeconds = 0.25;

// F9 presses from the window thread, taken by the game thread on the next player update.
class Requests {
 public:
  void PlayerUpdated(double now) {
    std::lock_guard lock(mutex_);
    last_update_ = now;
  }

  bool Request(double now) {
    std::lock_guard lock(mutex_);
    if (!Active(now)) {
      return false;
    }
    ++pending_;
    requested_at_ = now;
    return true;
  }

  uint32_t Take(double now) {
    std::lock_guard lock(mutex_);
    const uint32_t taken = now - requested_at_ <= kActiveSeconds ? pending_ : 0;
    pending_ = 0;
    return taken;
  }

 private:
  bool Active(double now) const { return last_update_ >= 0.0 && now - last_update_ <= kActiveSeconds; }

  std::mutex mutex_;
  double last_update_ = -1.0;
  double requested_at_ = -1.0;
  uint32_t pending_ = 0;
};

// Bombs F9 gave each player that are still in the player's count. Only these lift a mode's no-bombs rule.
// Game thread only.
class Grants {
 public:
  void Grant(uint32_t player, uint32_t bombs) {
    if (player < kMaxPlayers) {
      granted_[player] += bombs;
    }
  }

  // The count can drop without a bomb firing (a new game), so the grants never exceed it.
  bool LiftsNoBombsRule(uint32_t player, bool mode_refuses, uint32_t count) {
    if (player >= kMaxPlayers) {
      return false;
    }
    if (granted_[player] > count) {
      granted_[player] = count;
    }
    return mode_refuses && granted_[player] > 0;
  }

  void BombFired(uint32_t player) {
    if (player < kMaxPlayers && granted_[player] > 0) {
      --granted_[player];
    }
  }

  uint32_t Granted(uint32_t player) const { return player < kMaxPlayers ? granted_[player] : 0; }

 private:
  std::array<uint32_t, kMaxPlayers> granted_{};
};

// Called from the window's key handler for a new F9 press.
void RequestBomb();
// Called by the game thread from the player's update.
uint32_t TakeBombs();
Grants& GameGrants();

}  // namespace gw2::debug_bomb
