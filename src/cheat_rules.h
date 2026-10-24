#pragma once

#include <cstdint>

namespace gw2::cheats {

// The Player class vtable, stored in every player object.
inline constexpr uint32_t kPlayerVtable = 0x82001EB8;
inline constexpr uint32_t kPlayerIndexOffset = 0xA4;
// Seconds left of the short invulnerability a bomb gives, and of the shield.
inline constexpr uint32_t kPlayerBombGraceOffset = 0x88;
inline constexpr uint32_t kPlayerShieldOffset = 0x8C;

inline constexpr uint32_t kMaxPlayers = 4;
// Player 0's bomb count. The per-player records are 0x78 apart.
inline constexpr uint32_t kBombCountFirst = 0x8219D09C;
inline constexpr uint32_t kBombCountStride = 0x78;
// The shield time of the game's own Cheat.Shield.
inline constexpr float kShieldSeconds = 10.0f;

// The velocity Entity::Tick adds to the position, scaled by the frame time, after the entity's Update.
inline constexpr uint32_t kEntityVelocityXOffset = 0xC;
inline constexpr uint32_t kEntityVelocityYOffset = 0x10;

// The mode's starting bomb count and its rule flags. A mode that starts without bombs and has the no-bombs flag
// (Pacifism) refuses every bomb, whatever the player's count says.
inline constexpr uint32_t kModeStartBombsAddress = 0x8219D268;
inline constexpr uint32_t kModeFlagsAddress = 0x8219D26C;
inline constexpr uint32_t kModeNoBombsFlag = 0x2;

constexpr bool ModeRefusesBombs(int32_t start_bombs, uint32_t flags) {
  return start_bombs <= 0 && (flags & kModeNoBombsFlag) != 0;
}

// The speeds are powers of two, so scaling and unscaling the velocity is exact.
constexpr float ScaledVelocity(float velocity, int speed) {
  return velocity * static_cast<float>(speed);
}

constexpr float UnscaledVelocity(float velocity, int speed) {
  return velocity / static_cast<float>(speed);
}

constexpr uint32_t BombCountAddress(uint32_t player_index) {
  return player_index < kMaxPlayers ? kBombCountFirst + player_index * kBombCountStride : 0;
}

// The bomb function takes exactly one bomb when a bomb fires.
constexpr bool BombWasConsumed(uint32_t before, uint32_t after) {
  return before > 0 && after == before - 1;
}

// A hit kills a player that has neither bomb grace nor a shield. Neither check passes for NaN, as in the game.
constexpr bool HitWouldKill(float bomb_grace, float shield) {
  return !(bomb_grace > 0.0f) && !(shield > 0.0f);
}

// Hits that the shield or the bomb grace handle stay the game's own, so the shield keeps its normal behavior.
constexpr bool SuppressesHit(bool invincible, float bomb_grace, float shield) {
  return invincible && HitWouldKill(bomb_grace, shield);
}

constexpr float HeldShield(float shield) {
  return shield >= kShieldSeconds ? shield : kShieldSeconds;
}

}  // namespace gw2::cheats
