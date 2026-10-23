#include <bit>
#include <cstdint>

#include <rex/hook.h>

#include "add_lives.h"
#include "cheat_rules.h"
#include "cheats.h"
#include "debug_bomb.h"
#include "guest_memory.h"
#include "player_records.h"
#include "rumble.h"

namespace {

using gw2::cheats::Cheat;
using gw2::cheats::Enabled;

float LoadFloat(const uint8_t* base, uint32_t addr) {
  return std::bit_cast<float>(gw2::LoadBE32(base, addr));
}

void StoreFloat(uint8_t* base, uint32_t addr, float value) {
  gw2::StoreBE32(base, addr, std::bit_cast<uint32_t>(value));
}

using gw2::players::IsPlayer;

// The player Entity::Tick is running for, so Player::Update can scale the velocity that the tick then integrates.
struct MovementScale {
  uint32_t player = 0;
  int speed = gw2::cheats::kMovementSpeedOff;
  bool scaled = false;
};
thread_local MovementScale g_movement;

void ScaleVelocity(uint8_t* base, uint32_t object, float (*scale)(float, int), int speed) {
  for (const uint32_t offset : {gw2::cheats::kEntityVelocityXOffset, gw2::cheats::kEntityVelocityYOffset}) {
    StoreFloat(base, object + offset, scale(LoadFloat(base, object + offset), speed));
  }
}

void GrantDebugBombs(uint8_t* base, uint32_t index) {
  const uint32_t bombs = gw2::debug_bomb::TakeBombs();
  const uint32_t counter = gw2::cheats::BombCountAddress(index);
  if (bombs == 0 || counter == 0) {
    return;
  }
  gw2::StoreBE32(base, counter, gw2::LoadBE32(base, counter) + bombs);
  gw2::debug_bomb::GameGrants().Grant(index, bombs);
}

void GrantAddLives(uint8_t* base, uint32_t index) {
  const uint32_t uses = gw2::add_lives::Take();
  const uint32_t addr = gw2::players::LivesAddress(index);
  if (uses == 0 || addr == 0) {
    return;
  }
  const uint32_t current = gw2::LoadBE32(base, addr);
  const uint32_t add = uses * gw2::add_lives::kLivesPerUse;
  const uint32_t sum = current + add;
  gw2::StoreBE32(base, addr, sum < current ? UINT32_MAX : sum);
}

}  // namespace

// Entity::Tick: the entity's Update, then position += velocity * dt and the arena walls. The player's velocity is
// scaled only across that step, so its own acceleration and damping stay the game's.
REX_EXTERN(__imp__sub_820453E0);
REX_HOOK_RAW(sub_820453E0) {
  const uint32_t entity = ctx.r3.u32;
  const int speed = IsPlayer(base, entity) ? gw2::cheats::MovementSpeed() : gw2::cheats::kMovementSpeedOff;
  const bool moves_faster = speed != gw2::cheats::kMovementSpeedOff;
  if (moves_faster) {
    g_movement = {entity, speed, false};
  }
  __imp__sub_820453E0(ctx, base);
  if (!moves_faster) {
    return;
  }
  if (g_movement.scaled) {
    ScaleVelocity(base, entity, gw2::cheats::UnscaledVelocity, speed);
  }
  g_movement = {};
}

// Player::Update, once per frame per player. The shield countdown is inline in it, so the timer is held afterwards.
REX_EXTERN(__imp__sub_8204FF00);
REX_HOOK_RAW(sub_8204FF00) {
  const uint32_t player = ctx.r3.u32;
  const bool is_player = IsPlayer(base, player);
  const bool infinite = is_player && Enabled(Cheat::InfiniteShield);
  __imp__sub_8204FF00(ctx, base);
  if (is_player && g_movement.player == player && !g_movement.scaled) {
    ScaleVelocity(base, player, gw2::cheats::ScaledVelocity, g_movement.speed);
    g_movement.scaled = true;
  }
  if (is_player && gw2::LoadBE32(base, player + gw2::cheats::kPlayerIndexOffset) == 0) {
    GrantDebugBombs(base, 0);
    GrantAddLives(base, 0);
    gw2::rumble::ObserveLocalPlayer(LoadFloat(base, player + gw2::players::kCenterXOffset),
                                    LoadFloat(base, player + gw2::players::kCenterYOffset),
                                    LoadFloat(base, player + gw2::players::kRadiusOffset));
  }
  if (!infinite) {
    return;
  }
  const float shield = LoadFloat(base, player + gw2::cheats::kPlayerShieldOffset);
  const float held = gw2::cheats::HeldShield(shield);
  if (held != shield) {
    StoreFloat(base, player + gw2::cheats::kPlayerShieldOffset, held);
  }
}

// Player::OnHit, reached by every enemy, hazard and collision that can kill a player. The game's own Cheat.Invulnerable
// returns from here before anything else; this only does that for hits that would kill.
REX_EXTERN(__imp__sub_8204EEA8);
REX_HOOK_RAW(sub_8204EEA8) {
  const uint32_t player = ctx.r3.u32;
  const bool is_player = IsPlayer(base, player);
  if (is_player &&
      gw2::cheats::SuppressesHit(Enabled(Cheat::Invincible), LoadFloat(base, player + gw2::cheats::kPlayerBombGraceOffset),
                                 LoadFloat(base, player + gw2::cheats::kPlayerShieldOffset))) {
    return;
  }
  const uint32_t index = is_player ? gw2::LoadBE32(base, player + gw2::cheats::kPlayerIndexOffset) : 0;
  const uint32_t counter = is_player ? gw2::players::ShieldHitCountAddress(index) : 0;
  const uint32_t before = counter != 0 ? gw2::LoadBE32(base, counter) : 0;
  __imp__sub_8204EEA8(ctx, base);
  if (counter != 0 && gw2::LoadBE32(base, counter) == before + 1) {
    gw2::rumble::RequestShieldHitPulse(index);
  }
}

// Player bomb input, called by Player::Update. When a bomb fires it takes one from the count at the player's record.
REX_EXTERN(__imp__sub_8204F288);
REX_HOOK_RAW(sub_8204F288) {
  const uint32_t player = ctx.r3.u32;
  const bool is_player = IsPlayer(base, player);
  const uint32_t index = is_player ? gw2::LoadBE32(base, player + gw2::cheats::kPlayerIndexOffset) : 0;
  const uint32_t counter = is_player ? gw2::cheats::BombCountAddress(index) : 0;
  const bool infinite = counter != 0 && Enabled(Cheat::InfiniteBombs);
  const uint32_t before = counter != 0 ? gw2::LoadBE32(base, counter) : 0;
  const bool mode_refuses =
      gw2::cheats::ModeRefusesBombs(static_cast<int32_t>(gw2::LoadBE32(base, gw2::cheats::kModeStartBombsAddress)),
                                    gw2::LoadBE32(base, gw2::cheats::kModeFlagsAddress));
  auto& grants = gw2::debug_bomb::GameGrants();
  // Pacifism's no-bombs rule is lifted only for this call, and only while an F9 bomb is in the count.
  const bool lift = counter != 0 && grants.LiftsNoBombsRule(index, mode_refuses, before);
  const uint32_t flags = gw2::LoadBE32(base, gw2::cheats::kModeFlagsAddress);
  if (lift) {
    gw2::StoreBE32(base, gw2::cheats::kModeFlagsAddress, flags & ~gw2::cheats::kModeNoBombsFlag);
  }
  __imp__sub_8204F288(ctx, base);
  if (lift) {
    gw2::StoreBE32(base, gw2::cheats::kModeFlagsAddress,
                   gw2::LoadBE32(base, gw2::cheats::kModeFlagsAddress) | gw2::cheats::kModeNoBombsFlag);
  }
  if (counter == 0 || !gw2::cheats::BombWasConsumed(before, gw2::LoadBE32(base, counter))) {
    return;
  }
  grants.BombFired(index);
  gw2::rumble::RequestBombPulse(index);
  if (infinite) {
    gw2::StoreBE32(base, counter, before);
  }
}
