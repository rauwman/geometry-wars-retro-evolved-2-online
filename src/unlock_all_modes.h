#pragma once

#include <cstdint>

namespace gw2::unlock_all_modes {

inline constexpr char kKey[] = "gw2_unlock_all_game_modes";
inline constexpr bool kDefault = false;
inline constexpr char kLabel[] = "Unlock all game modes";
inline constexpr char kDescription[] = "Makes all game modes available without changing saved progression";

// The local player's live profile record (see docs/offline-live.md): a 48-byte table entry per user, found from
// the recompiled game code that loads/saves the profile (sub_8207F5B8, sub_820E8A20) through the function that
// indexes it (sub_82093600). Word 4 (byte offset 16) is the mask of unlocked modes; bits 1/2/4/8/16/32 are
// Evolved/Deadline/Pacifism/Waves/King/Sequence, confirmed from the game's own "unlock everything" debug routine
// (sub_8208D218), which also resets words 5-10 (offsets 20-40, the per-mode progression) - exactly the behavior
// this cheat must not reproduce.
inline constexpr uint32_t kProfileTableAddress = 0x821F47B8;
inline constexpr uint32_t kProfileRecordSize = 48;
inline constexpr uint32_t kMaskOffset = 16;
inline constexpr uint32_t kAllModesMask = 0x3F;

constexpr uint32_t MaskAddress(uint32_t user_index = 0) {
  return kProfileTableAddress + user_index * kProfileRecordSize + kMaskOffset;
}

// What the live mask should read this call, what to remember as the real (saved) mask, and whether the live mask
// is now the artificial one. `was_overriding` is the `overriding` this same function returned last call (false
// initially), not inferred from the mask value: a real mask that happens to already be kAllModesMask must never be
// mistaken for an override, and must never be replaced by a stale remembered value because of that.
//
// While off, the real mask simply tracks whatever is actually there. The instant it turns on, the real mask is
// captured from the current mask and then stays frozen until the cheat turns off again, when it is restored
// exactly as captured.
struct SyncResult {
  uint32_t next_mask;
  uint32_t next_real_mask;
  bool overriding;
};

constexpr SyncResult Sync(uint32_t current_mask, bool enabled, uint32_t real_mask, bool was_overriding) {
  if (enabled) {
    return {kAllModesMask, was_overriding ? real_mask : current_mask, true};
  }
  if (was_overriding) {
    return {real_mask, real_mask, false};
  }
  return {current_mask, current_mask, false};
}

// The mask that must be in the live profile right before any profile write, so the artificial mask can never be
// persisted: the real one while overriding, otherwise whatever is already there.
constexpr uint32_t MaskForWrite(bool overriding, uint32_t real_mask, uint32_t current_mask) {
  return overriding ? real_mask : current_mask;
}

// Whether the saved setting (or a launcher/options-menu edit already applied to it) is on.
bool Enabled();

// Applies Sync() to the local player's live profile record. Called once per guest frame and right after the
// profile loads.
void ServiceFrame(uint8_t* base);

// Applies MaskForWrite() to the local player's live profile record. Called right before the game writes its
// profile.
void RestoreForWrite(uint8_t* base);

}  // namespace gw2::unlock_all_modes
