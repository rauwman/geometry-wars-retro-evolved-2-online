#pragma once

#include <cstdint>
#include <mutex>

namespace gw2::add_lives {

inline constexpr uint32_t kLivesPerUse = 100;

// A menu activation from the window thread, taken by the game thread on the local player's next update.
class Requests {
 public:
  void Request() {
    std::lock_guard lock(mutex_);
    ++pending_;
  }

  uint32_t Take() {
    std::lock_guard lock(mutex_);
    const uint32_t taken = pending_;
    pending_ = 0;
    return taken;
  }

 private:
  std::mutex mutex_;
  uint32_t pending_ = 0;
};

// Called from the cheat menu row when Add 100 Lives is activated.
void Request();
// Called by the game thread from the local player's update. Returns how many uses are pending.
uint32_t Take();

}  // namespace gw2::add_lives
