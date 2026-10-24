#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/hook.h>

#include "player_names.h"

// sub_8207DD78 loops over the 4 player records every frame, re-reading XamUserGetName only when a
// record's cached XUID differs from the user's current one. An unsigned/guest slot's XUID is always 0,
// so it never differs after the first frame: players 2-4 keep whatever name that first frame resolved
// (the "Player N" fallback), even once a custom name is configured later. Re-stamp both of each
// record's cached name buffers afterward so a configured name still takes effect; a default (unconfigured)
// slot is left exactly as the game wrote it.
REX_EXTERN(__imp__sub_8207DD78);
REX_HOOK_RAW(sub_8207DD78) {
  __imp__sub_8207DD78(ctx, base);
  for (uint32_t i = 0; i < gw2::kPlayerNames.size(); ++i) {
    char name[16];
    const std::string configured = rex::cvar::GetFlagByName(gw2::kPlayerNames[i].sdk_key);
    if (!gw2::CachedPlayerNameBytes(configured, name)) {
      continue;
    }
    std::memcpy(base + gw2::PlayerRecordNameAddress(i, 0), name, sizeof name);
    std::memcpy(base + gw2::PlayerRecordNameAddress(i, 1), name, sizeof name);
  }
}
