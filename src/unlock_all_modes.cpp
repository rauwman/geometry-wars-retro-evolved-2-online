#include "unlock_all_modes.h"

#include <mutex>

#include <rex/cvar.h>
#include <rex/hook.h>

#include "guest_memory.h"

REXCVAR_DEFINE_BOOL(gw2_unlock_all_game_modes, gw2::unlock_all_modes::kDefault, "Geometry Wars",
                    "Makes all game modes available without changing saved progression");

namespace gw2::unlock_all_modes {
namespace {

// Keeps the real mask remembered across calls, and applies Sync/MaskForWrite to the live profile.
class LiveOverride {
 public:
  void Apply(uint8_t* base, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint32_t addr = MaskAddress();
    const uint32_t current = LoadBE32(base, addr);
    const SyncResult result = Sync(current, enabled, real_mask_, overriding_);
    real_mask_ = result.next_real_mask;
    overriding_ = result.overriding;
    if (result.next_mask != current) {
      StoreBE32(base, addr, result.next_mask);
    }
  }

  void RestoreForWrite(uint8_t* base) {
    std::lock_guard<std::mutex> lock(mutex_);
    const uint32_t addr = MaskAddress();
    const uint32_t current = LoadBE32(base, addr);
    const uint32_t safe = MaskForWrite(overriding_, real_mask_, current);
    if (safe != current) {
      StoreBE32(base, addr, safe);
    }
  }

 private:
  std::mutex mutex_;
  uint32_t real_mask_ = 0;
  bool overriding_ = false;
};

LiveOverride g_override;

}  // namespace

bool Enabled() {
  return REXCVAR_GET(gw2_unlock_all_game_modes);
}

void ServiceFrame(uint8_t* base) {
  g_override.Apply(base, Enabled());
}

void RestoreForWrite(uint8_t* base) {
  g_override.RestoreForWrite(base);
}

}  // namespace gw2::unlock_all_modes

// The game's own function that parses the loaded title setting into the local player's live profile record
// (memcpy of the 48 saved bytes). Applying the cheat right after a load covers both the first load and any later
// reload (e.g. the game's own "reset profile" action) without waiting for the next frame.
REX_EXTERN(__imp__sub_8207F5B8);
REX_HOOK_RAW(sub_8207F5B8) {
  __imp__sub_8207F5B8(ctx, base);
  gw2::unlock_all_modes::ServiceFrame(base);
}

// Wraps XamUserWriteProfileSettings for the local player's profile. The write reads the live profile record
// directly, so the real mask must be in place for the duration of this call; it is forced back to the artificial
// mask afterward (if the cheat is still on) so play continues unaffected.
REX_EXTERN(__imp__sub_820E8A20);
REX_HOOK_RAW(sub_820E8A20) {
  gw2::unlock_all_modes::RestoreForWrite(base);
  __imp__sub_820E8A20(ctx, base);
  gw2::unlock_all_modes::ServiceFrame(base);
}
