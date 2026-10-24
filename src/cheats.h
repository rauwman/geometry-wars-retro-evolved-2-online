#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>

namespace gw2::cheats {

enum class Cheat : uint8_t { Invincible, InfiniteShield, InfiniteBombs };
inline constexpr size_t kCheatCount = 3;
inline constexpr bool kCheatDefault = false;

inline constexpr char kInvincibleKey[] = "gw2_cheat_invincible";
inline constexpr char kInfiniteShieldKey[] = "gw2_cheat_infinite_shield";
inline constexpr char kInfiniteBombsKey[] = "gw2_cheat_infinite_bombs";

// The one list the launcher page, the in-game menu and the settings are built from.
struct CheatInfo {
  Cheat cheat;
  const char* key;
  const char* label;
  const char* description;
};

inline constexpr CheatInfo kCheats[] = {
    {Cheat::Invincible, kInvincibleKey, "Invincible", "Prevents the player from dying"},
    {Cheat::InfiniteShield, kInfiniteShieldKey, "Infinite Shield", "Keeps the player's protective shield active"},
    {Cheat::InfiniteBombs, kInfiniteBombsKey, "Infinite Bombs", "Bombs are not consumed when used"},
};
static_assert(std::size(kCheats) == kCheatCount);

// Movement Speed is a choice rather than an on/off cheat. Its value is the player's speed multiplier.
inline constexpr char kMovementSpeedKey[] = "gw2_cheat_movement_speed";
inline constexpr char kMovementSpeedLabel[] = "Movement Speed";
inline constexpr char kMovementSpeedDescription[] = "Multiplies how fast the player's ship moves";
inline constexpr int kMovementSpeedOff = 1;
inline constexpr int kMovementSpeedDefault = kMovementSpeedOff;

struct MovementSpeedChoice {
  int value;
  const char* label;
};

inline constexpr MovementSpeedChoice kMovementSpeeds[] = {{1, "Off"}, {2, "2x"}, {4, "4x"}};

// Any value that is not one of the choices, as from a hand-edited config, is Off.
constexpr int ParseMovementSpeed(int value) {
  for (const MovementSpeedChoice& choice : kMovementSpeeds) {
    if (choice.value == value) {
      return value;
    }
  }
  return kMovementSpeedOff;
}

constexpr int NextMovementSpeed(int value) {
  const int current = ParseMovementSpeed(value);
  for (size_t i = 0; i < std::size(kMovementSpeeds); ++i) {
    if (kMovementSpeeds[i].value == current) {
      return kMovementSpeeds[(i + 1) % std::size(kMovementSpeeds)].value;
    }
  }
  return kMovementSpeedOff;
}

constexpr const char* MovementSpeedLabel(int value) {
  const int current = ParseMovementSpeed(value);
  for (const MovementSpeedChoice& choice : kMovementSpeeds) {
    if (choice.value == current) {
      return choice.label;
    }
  }
  return kMovementSpeeds[0].label;
}

// The launcher's saved value of a cheat, or the in-game menu's override of it for this session.
// Defined in cheats.cpp.
bool Enabled(Cheat cheat);
// Never writes the saved settings.
void ToggleForSession(Cheat cheat);

// The same as Enabled, for the game's own hooks: an enabled cheat that the game consults counts as used.
bool EnabledInGame(Cheat cheat);
// Whether a cheat was in effect since the last call, or is enabled now. The next score is then not a clean one.
// Sticky between calls, so toggling a cheat off again does not clear it.
bool TakeUsed();
// The saved Movement Speed, or the in-game menu's override of it for this session.
int MovementSpeed();
// Off, 2x, 4x, Off. Never writes the saved settings.
void CycleMovementSpeedForSession();

// What the in-game menu changed. It only records differences from the saved value, so the saved value is
// never altered and a cheat toggled back follows the saved value again.
class SessionCheats {
 public:
  bool Effective(Cheat cheat, bool saved) const {
    const uint8_t state = states_[Index(cheat)].load(std::memory_order_relaxed);
    return state == kFollowsSaved ? saved : state == kOn;
  }

  void Set(Cheat cheat, bool saved, bool enabled) {
    states_[Index(cheat)].store(enabled == saved ? kFollowsSaved : (enabled ? kOn : kOff),
                                std::memory_order_relaxed);
  }

  void Toggle(Cheat cheat, bool saved) { Set(cheat, saved, !Effective(cheat, saved)); }

 private:
  static constexpr uint8_t kFollowsSaved = 0;
  static constexpr uint8_t kOff = 1;
  static constexpr uint8_t kOn = 2;

  static size_t Index(Cheat cheat) { return static_cast<size_t>(cheat); }

  std::array<std::atomic<uint8_t>, kCheatCount> states_{};
};

// The Movement Speed counterpart of SessionCheats: an override only while it differs from the saved value.
class SessionMovementSpeed {
 public:
  int Effective(int saved) const {
    const int state = state_.load(std::memory_order_relaxed);
    return state == kFollowsSaved ? ParseMovementSpeed(saved) : state;
  }

  void Set(int saved, int value) {
    const int parsed = ParseMovementSpeed(value);
    state_.store(parsed == ParseMovementSpeed(saved) ? kFollowsSaved : parsed, std::memory_order_relaxed);
  }

  void Cycle(int saved) { Set(saved, NextMovementSpeed(Effective(saved))); }

 private:
  static constexpr int kFollowsSaved = 0;

  std::atomic<int> state_{kFollowsSaved};
};

}  // namespace gw2::cheats
