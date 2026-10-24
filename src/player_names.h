#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "user_display_name.h"

namespace gw2 {

struct PlayerNameSetting {
  const char* key;
  const char* sdk_key;
  const char* label;
  const char* default_name;
  bool send_default;
};

inline constexpr char kPlayerNameHint[] = "Up to 15 characters.";
inline constexpr char kPlayerNamesHelp[] = "Changes the name shown in-game. Save data is not affected.";

inline constexpr std::array<PlayerNameSetting, 4> kPlayerNames = {{
    {"gw2_player_1_name", "local_player_1_name", "Player 1", "Player 1", true},
    {"gw2_player_2_name", "local_player_2_name", "Player 2", "Player 2", false},
    {"gw2_player_3_name", "local_player_3_name", "Player 3", "Player 3", false},
    {"gw2_player_4_name", "local_player_4_name", "Player 4", "Player 4", false},
}};

using PlayerNameResult = rex::system::xam::UserDisplayNameResult;

inline bool IsValidPlayerName(std::string_view text) {
  return rex::system::xam::NormalizeUserDisplayName(text, nullptr);
}

inline PlayerNameResult CheckPlayerName(std::string_view raw) {
  const std::string_view trimmed = rex::system::xam::TrimUserDisplayName(raw);
  return rex::system::xam::MeasureUserDisplayName(trimmed);
}

// The value for the SDK's local_player_N_name cvar. An invalid name becomes the default. A user that is not signed
// in only gets a name once it differs from the default; empty leaves the SDK answering "no such user".
inline std::string SdkPlayerName(const PlayerNameSetting& player, std::string_view launcher_value) {
  std::string name;
  if (!rex::system::xam::NormalizeUserDisplayName(launcher_value, &name)) {
    name = player.default_name;
  }
  if (!player.send_default && name == player.default_name) {
    return {};
  }
  return name;
}

// The 16-byte buffer the game caches a local player's display name in (the same ABI as XamUserGetName).
// Returns false and leaves out untouched when no custom name is configured, so the game's own default
// result (whatever it already wrote there) is left alone.
inline bool CachedPlayerNameBytes(std::string_view configured_sdk_value, char out[16]) {
  if (configured_sdk_value.empty()) {
    return false;
  }
  std::memset(out, 0, 16);
  const size_t length = rex::system::xam::Utf8PrefixLength(configured_sdk_value,
                                                            rex::system::xam::kUserDisplayNameMaxBytes);
  std::memcpy(out, configured_sdk_value.data(), length);
  return true;
}

// sub_8207DD78's per-local-player record table: a fixed global array of 4 records, 104 bytes apart,
// read every frame to drive the Multiplayer screen's P1-P4 name panels. Each record holds the display
// name in two 16-byte buffers, at offsets 12 and 28 of the record.
inline constexpr uint32_t kPlayerRecordBase = 0x821D37F0;
inline constexpr uint32_t kPlayerRecordStride = 104;
inline constexpr uint32_t kPlayerRecordNameOffsets[2] = {12, 28};

inline constexpr uint32_t PlayerRecordNameAddress(uint32_t index, int which) {
  return kPlayerRecordBase + index * kPlayerRecordStride + kPlayerRecordNameOffsets[which];
}

inline const char* PlayerNameMessage(PlayerNameResult result) {
  switch (result) {
    case PlayerNameResult::Empty:
      return "Enter a name.";
    case PlayerNameResult::TooManyCharacters:
      return "Maximum 15 characters.";
    case PlayerNameResult::TooManyBytes:
      return "Name is too long for the game (15-byte limit).";
    case PlayerNameResult::BadCharacter:
      return "That name has a character that cannot be used.";
    default:
      return "";
  }
}

}  // namespace gw2
