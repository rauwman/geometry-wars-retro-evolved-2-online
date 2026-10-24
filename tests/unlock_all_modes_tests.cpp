// Unlock all game modes: the profile-mask override logic (pure, header-only) and how the launcher, the in-game
// Options menu and the guest hooks are wired to the one setting. Logic and source checks only; the hooks
// themselves need the running runtime.

#include <algorithm>
#include "unlock_all_modes.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
using namespace gw2::unlock_all_modes;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                 \
  do {                                                              \
    ++g_checks;                                                     \
    if (!(cond)) {                                                  \
      ++g_failures;                                                 \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
    }                                                                \
  } while (0)

void Section(const char* name) {
  std::printf("%s\n", name);
}

#ifdef GW2_SRC_DIR
const fs::path kSrcDir = GW2_SRC_DIR;
#else
const fs::path kSrcDir = fs::path(__FILE__).parent_path().parent_path() / "src";
#endif

std::string ReadSource(const std::string& name) {
  std::ifstream in(kSrcDir / name, std::ios::binary);
  std::ostringstream text;
  text << in.rdbuf();
  // A Windows checkout may have CRLF line endings; the tests match LF text.
  std::string source = text.str();
  source.erase(std::remove(source.begin(), source.end(), '\r'), source.end());
  return source;
}

int Count(const std::string& text, const std::string& needle) {
  int count = 0;
  for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + needle.size())) {
    ++count;
  }
  return count;
}

void TestKeyAndDefault() {
  Section("key, default and wording");
  CHECK(std::string_view(kKey) == "gw2_unlock_all_game_modes");
  CHECK(!kDefault);
  CHECK(std::string_view(kLabel) == "Unlock all game modes");
  CHECK(std::string_view(kDescription) == "Makes all game modes available without changing saved progression");
}

void TestProfileAddressing() {
  Section("profile record addressing");
  CHECK(kProfileTableAddress == 0x821F47B8);
  CHECK(kProfileRecordSize == 48);
  CHECK(kMaskOffset == 16);
  CHECK(kAllModesMask == 0x3F);
  CHECK(MaskAddress(0) == 0x821F47C8);
  CHECK(MaskAddress(1) == 0x821F47C8 + 48);
  CHECK(MaskAddress(3) == 0x821F47C8 + 48 * 3);
}

void TestSyncOffTracksTheRealMask() {
  Section("off: the real mask is used unchanged");
  for (const uint32_t locked : {0u, 2u, 7u, 0x3Fu}) {
    const SyncResult result = Sync(locked, /*enabled=*/false, /*real_mask=*/0xDEAD, /*was_overriding=*/false);
    CHECK(result.next_mask == locked);
    CHECK(result.next_real_mask == locked);
    CHECK(!result.overriding);
  }
}

void TestSyncOnForcesAllModesAndFreezesTheRealMask() {
  Section("on: the effective mask becomes 0x3F and the real mask is captured once");
  const SyncResult first = Sync(/*current=*/2, /*enabled=*/true, /*real_mask=*/0xDEAD, /*was_overriding=*/false);
  CHECK(first.next_mask == kAllModesMask);
  CHECK(first.next_real_mask == 2);
  CHECK(first.overriding);

  // While it stays on, the real mask stays frozen at 2 even though the live mask now reads 0x3F.
  const SyncResult again = Sync(/*current=*/kAllModesMask, /*enabled=*/true, /*real_mask=*/2, /*was_overriding=*/true);
  CHECK(again.next_mask == kAllModesMask);
  CHECK(again.next_real_mask == 2);
  CHECK(again.overriding);
}

void TestTurningOffRestoresTheRealMask() {
  Section("turning off restores the real mask, not the artificial one");
  const SyncResult restored = Sync(/*current=*/kAllModesMask, /*enabled=*/false, /*real_mask=*/2, /*was_overriding=*/true);
  CHECK(restored.next_mask == 2);
  CHECK(restored.next_real_mask == 2);
  CHECK(!restored.overriding);
}

void TestGenuinelyFullyUnlockedIsNotMistakenForTheOverride() {
  Section("a real mask that is already 0x3F is not mistaken for the override");
  // Off, and the real mask already happens to be 0x3F: must not be replaced by a stale remembered value.
  const SyncResult off = Sync(/*current=*/kAllModesMask, /*enabled=*/false, /*real_mask=*/0xDEAD, /*was_overriding=*/false);
  CHECK(off.next_mask == kAllModesMask);
  CHECK(off.next_real_mask == kAllModesMask);

  // Turning the cheat on from there is a no-op: it captures 0x3F as the real mask, not whatever was remembered.
  const SyncResult on = Sync(/*current=*/kAllModesMask, /*enabled=*/true, /*real_mask=*/off.next_real_mask,
                             /*was_overriding=*/off.overriding);
  CHECK(on.next_mask == kAllModesMask);
  CHECK(on.next_real_mask == kAllModesMask);

  // Turning it back off restores 0x3F, which is genuinely correct here.
  const SyncResult back_off = Sync(/*current=*/kAllModesMask, /*enabled=*/false, /*real_mask=*/on.next_real_mask,
                                   /*was_overriding=*/on.overriding);
  CHECK(back_off.next_mask == kAllModesMask);
}

void TestOnlyTheMaskIsEverInvolved() {
  Section("only word 4 is involved; nothing else is touched by this logic");
  const std::string hooks = ReadSource("unlock_all_modes.cpp");
  // Every guest-memory access in the implementation goes through MaskAddress(); no other profile offset (the
  // words 5-10 progression offsets are 20, 24, 28, 32, 36, 40) is ever read or written.
  CHECK(Count(hooks, "LoadBE32(base,") == Count(hooks, "LoadBE32(base, addr)"));
  CHECK(Count(hooks, "StoreBE32(base,") == Count(hooks, "StoreBE32(base, addr,"));
  for (const char* offset : {", 20)", ", 24)", ", 28)", ", 32)", ", 36)", ", 40)"}) {
    CHECK(hooks.find(offset) == std::string::npos);
  }
}

void TestMaskForWriteNeverReturnsTheArtificialMaskUnlessItIsReal() {
  Section("0x3F is never the mask a write uses unless it is genuinely the real mask");
  // Not overriding: whatever is already there is safe to write as-is.
  CHECK(MaskForWrite(/*overriding=*/false, /*real_mask=*/2, /*current_mask=*/2) == 2);
  CHECK(MaskForWrite(/*overriding=*/false, /*real_mask=*/2, /*current_mask=*/kAllModesMask) == kAllModesMask);

  // Overriding: the write must use the real mask, never the live (artificial) one.
  CHECK(MaskForWrite(/*overriding=*/true, /*real_mask=*/2, /*current_mask=*/kAllModesMask) == 2);
  CHECK(MaskForWrite(/*overriding=*/true, /*real_mask=*/2, /*current_mask=*/2) == 2);
}

void TestRoundTrip() {
  Section("off -> on -> write -> off round trip");
  uint32_t real = 0;
  bool overriding = false;
  const uint32_t locked = 2;

  // Starts off, locked.
  SyncResult step = Sync(locked, false, real, overriding);
  real = step.next_real_mask;
  overriding = step.overriding;
  CHECK(step.next_mask == locked);

  // Turns on: effective mask is 0x3F; the real mask captured is the locked value.
  step = Sync(step.next_mask, true, real, overriding);
  real = step.next_real_mask;
  overriding = step.overriding;
  CHECK(step.next_mask == kAllModesMask);
  CHECK(real == locked);

  // A profile write happens while on: MaskForWrite must hand back the locked value, not 0x3F.
  CHECK(MaskForWrite(overriding, real, step.next_mask) == locked);

  // After the write, the live mask is forced back to 0x3F for continued play.
  step = Sync(locked, true, real, overriding);
  CHECK(step.next_mask == kAllModesMask);
  CHECK(step.next_real_mask == locked);

  // Turns off again: the live mask returns to the real (locked) value, not 0x3F.
  step = Sync(step.next_mask, false, step.next_real_mask, step.overriding);
  CHECK(step.next_mask == locked);
  CHECK(!step.overriding);
}

void TestProfileReloadWhileOnRefreshesTheRealMask() {
  Section("reloading the profile while on refreshes the preserved real mask, not the stale one");
  uint32_t real = 0;
  bool overriding = false;
  const uint32_t mask_a = 2;

  // Loads mask A with the cheat off, then turns it on: A is captured, the effective mask becomes 0x3F.
  SyncResult step = Sync(mask_a, /*enabled=*/false, real, overriding);
  real = step.next_real_mask;
  overriding = step.overriding;
  step = Sync(step.next_mask, /*enabled=*/true, real, overriding);
  real = step.next_real_mask;
  overriding = step.overriding;
  CHECK(step.next_mask == kAllModesMask);
  CHECK(real == mask_a);

  // The profile-load path reloads a different real mask B while the cheat stays on. OnProfileLoaded always
  // resyncs with was_overriding forced to false, so B is captured as real instead of the stale A.
  const uint32_t mask_b = 5;
  step = Sync(mask_b, /*enabled=*/true, real, /*was_overriding=*/false);
  real = step.next_real_mask;
  overriding = step.overriding;
  CHECK(real == mask_b);                    // the preserved mask is now B
  CHECK(step.next_mask == kAllModesMask);   // the effective mask after the reload is still 0x3F
  CHECK(overriding);

  // A save right after the reload, while still on, must serialize B, never the artificial mask.
  CHECK(MaskForWrite(overriding, real, step.next_mask) == mask_b);

  // Turning the cheat off now restores B, not the stale A.
  step = Sync(step.next_mask, /*enabled=*/false, real, overriding);
  CHECK(step.next_mask == mask_b);
  CHECK(!step.overriding);
}

void TestRepeatedReloadsWhileOnAlwaysTrackTheLatestGenuineMask() {
  Section("repeated reloads while on always track the latest genuine mask");
  uint32_t real = 10;
  for (const uint32_t loaded : {3u, 9u, 0u, 0x15u}) {
    const SyncResult step = Sync(loaded, /*enabled=*/true, real, /*was_overriding=*/false);
    CHECK(step.next_real_mask == loaded);
    CHECK(step.next_mask == kAllModesMask);
    CHECK(step.overriding);
    real = step.next_real_mask;
  }
}

void TestGenuineAllModesMaskSurvivesAReloadWhileOn() {
  Section("a genuinely-0x3F reloaded mask while on is tracked as real, not mistaken for the artificial one");
  // The cheat is already on, having frozen some other real mask.
  const SyncResult reloaded = Sync(kAllModesMask, /*enabled=*/true, /*real_mask=*/2, /*was_overriding=*/false);
  CHECK(reloaded.next_real_mask == kAllModesMask);
  CHECK(reloaded.next_mask == kAllModesMask);

  // Turning the cheat off must restore the genuinely-0x3F mask just loaded, not the earlier frozen one.
  const SyncResult off = Sync(reloaded.next_mask, /*enabled=*/false, reloaded.next_real_mask, reloaded.overriding);
  CHECK(off.next_mask == kAllModesMask);
  CHECK(!off.overriding);
}

void TestWiredIntoLauncherAndOptionsMenu() {
  Section("the same setting is used by the launcher and the in-game Options menu");
  const std::string launcher = ReadSource("launcher.cpp");
  CHECK(Count(launcher, "unlock_all_modes::kKey") == 1);
  CHECK(Count(launcher, "unlock_all_modes::kLabel") == 1);
  CHECK(launcher.find("PageGameplay(trg::Ui") < launcher.find("unlock_all_modes::kKey"));

  const std::string options = ReadSource("options_menu.cpp");
  CHECK(Count(options, "unlock_all_modes::kKey") == 1);
  CHECK(Count(options, "unlock_all_modes::kLabel") == 1);
  const size_t gameplay_page = options.find("Page gameplay{std::string(PageName(PageId::kGameplay))");
  const size_t controls_page = options.find("Page controls{std::string(PageName(PageId::kControls))");
  const size_t row = options.find("unlock_all_modes::kKey");
  CHECK(gameplay_page != std::string::npos && controls_page != std::string::npos && row != std::string::npos);
  CHECK(gameplay_page < row && row < controls_page);

  const std::string settings = ReadSource("launcher_settings.cpp");
  CHECK(Count(settings, "{unlock_all_modes::kKey, ValueKind::kBool}") == 1);

  // Not a session-only cheat-menu toggle: it is a plain managed setting like Frame rate in title.
  CHECK(ReadSource("cheat_menu.cpp").find("unlock_all_modes") == std::string::npos);
  CHECK(ReadSource("cheats.h").find("unlock_all_modes") == std::string::npos);
}

void TestHooksAreWired() {
  Section("the profile-load and profile-write hooks exist exactly once");
  const std::string hooks = ReadSource("unlock_all_modes.cpp");
  CHECK(!hooks.empty());
  for (const char* name : {"sub_8207F5B8", "sub_820E8A20"}) {
    CHECK(Count(hooks, std::string("REX_HOOK_RAW(") + name + ")") == 1);
    CHECK(Count(hooks, std::string("REX_EXTERN(__imp__") + name + ")") == 1);
    CHECK(Count(hooks, std::string("__imp__") + name + "(ctx, base)") == 1);
  }
  CHECK(Count(hooks, "gw2::unlock_all_modes::RestoreForWrite(base)") == 1);
  CHECK(Count(hooks, "gw2::unlock_all_modes::OnProfileLoaded(base)") == 1);
  // The load hook must use OnProfileLoaded, not the plain per-frame ServiceFrame, so a reload while the cheat is
  // already on is always captured as a genuinely new real mask.
  CHECK(hooks.find("__imp__sub_8207F5B8(ctx, base);\n  gw2::unlock_all_modes::OnProfileLoaded(base);") !=
        std::string::npos);
  CHECK(Count(hooks, "ServiceFrame(base)") == 1);

  const std::string window_host = ReadSource("window_host.cpp");
  CHECK(Count(window_host, "gw2::unlock_all_modes::ServiceFrame(base)") == 1);
}

}  // namespace

int main() {
  TestKeyAndDefault();
  TestProfileAddressing();
  TestSyncOffTracksTheRealMask();
  TestSyncOnForcesAllModesAndFreezesTheRealMask();
  TestTurningOffRestoresTheRealMask();
  TestGenuinelyFullyUnlockedIsNotMistakenForTheOverride();
  TestOnlyTheMaskIsEverInvolved();
  TestMaskForWriteNeverReturnsTheArtificialMaskUnlessItIsReal();
  TestRoundTrip();
  TestProfileReloadWhileOnRefreshesTheRealMask();
  TestRepeatedReloadsWhileOnAlwaysTrackTheLatestGenuineMask();
  TestGenuineAllModesMaskSurvivesAReloadWhileOn();
  TestWiredIntoLauncherAndOptionsMenu();
  TestHooksAreWired();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
