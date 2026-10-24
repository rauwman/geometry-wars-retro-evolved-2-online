// Player names: the one validator and its edit gate, the launcher keys and page, the SDK values they produce,
// and that the display name never reaches the profile identity (XUID, content directory).
// Run: player_names_tests

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "player_names.h"

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                 \
  do {                                                              \
    ++g_checks;                                                     \
    if (!(cond)) {                                                  \
      ++g_failures;                                                 \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
    }                                                               \
  } while (0)

using rex::system::xam::AcceptUserDisplayNameEdit;
using rex::system::xam::NormalizeUserDisplayName;
using rex::system::xam::UserDisplayNameResult;

std::string ReadFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

std::string ReadSource(const char* name) {
  return ReadFile(std::string(GW2_SRC_DIR) + "/" + name);
}

std::string ReadSdkPatch() {
  return ReadFile(std::string(GW2_SRC_DIR) + "/../patches/rexglue-sdk/0054-local-user-display-names.patch");
}

bool Has(const std::string& text, const char* needle) {
  return text.find(needle) != std::string::npos;
}

size_t Count(const std::string& text, const char* needle) {
  size_t count = 0;
  for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) {
    ++count;
  }
  return count;
}

std::string Normalized(const std::string& raw) {
  std::string out = "<rejected>";
  NormalizeUserDisplayName(raw, &out);
  return out;
}

// What the name field holds after typing, pasting or deleting: the proposed text only when it is allowed.
std::string Edit(const std::string& previous, const std::string& proposed, UserDisplayNameResult* result = nullptr) {
  return AcceptUserDisplayNameEdit(previous, proposed, result);
}

const std::string kAccent = "\xC3\xA9";

std::string Repeat(const std::string& unit, int count) {
  std::string out;
  for (int i = 0; i < count; ++i) {
    out += unit;
  }
  return out;
}

void TestValidator() {
  std::printf("validator: 15 characters and 15 bytes in the 16-byte XAM buffer\n");
  CHECK(rex::system::xam::kUserNameBufferSize == 16);
  CHECK(rex::system::xam::kUserDisplayNameMaxBytes == 15);
  CHECK(rex::system::xam::kUserDisplayNameMaxCharacters == 15);
  CHECK(Normalized("Saulo") == "Saulo");
  CHECK(Normalized("  Saulo  ") == "Saulo");
  CHECK(Normalized("") == "<rejected>");
  CHECK(Normalized("    ") == "<rejected>");
  CHECK(Normalized(std::string(15, 'a')) == std::string(15, 'a'));
  CHECK(Normalized(std::string(16, 'a')) == "<rejected>");
  CHECK(Normalized(" " + std::string(15, 'a') + " ") == std::string(15, 'a'));
  CHECK(Normalized(Repeat(kAccent, 7) + "a") == Repeat(kAccent, 7) + "a");
  CHECK(Normalized(Repeat(kAccent, 8)) == "<rejected>");
  CHECK(Normalized("a\xC3") == "<rejected>");
  CHECK(Normalized("\xFF") == "<rejected>");
  CHECK(Normalized("\xC0\x80") == "<rejected>");
  CHECK(Normalized("\xED\xA0\x80") == "<rejected>");
  CHECK(Normalized("\xF4\x90\x80\x80") == "<rejected>");
  CHECK(Normalized(std::string("a\0b", 3)) == "<rejected>");
  CHECK(Normalized("a\tb") == "<rejected>");
  CHECK(Normalized("a\x7F") == "<rejected>");
  CHECK(Normalized("a\xC2\x85") == "<rejected>");
  CHECK(rex::system::xam::Utf8PrefixLength("ab\xC3\xA9", 3) == 2);
  CHECK(rex::system::xam::Utf8PrefixLength("ab\xC3\xA9", 4) == 4);
}

void TestMessages() {
  std::printf("friendly messages\n");
  CHECK(gw2::CheckPlayerName(std::string(15, 'a')) == gw2::PlayerNameResult::Ok);
  CHECK(gw2::CheckPlayerName(std::string(16, 'a')) == gw2::PlayerNameResult::TooManyCharacters);
  CHECK(std::string(gw2::PlayerNameMessage(gw2::PlayerNameResult::TooManyCharacters)) == "Maximum 15 characters.");
  CHECK(gw2::CheckPlayerName(Repeat(kAccent, 8)) == gw2::PlayerNameResult::TooManyBytes);
  CHECK(std::string(gw2::PlayerNameMessage(gw2::PlayerNameResult::TooManyBytes)) ==
        "Name is too long for the game (15-byte limit).");
  CHECK(gw2::CheckPlayerName("a\tb") == gw2::PlayerNameResult::BadCharacter);
  CHECK(gw2::CheckPlayerName("\xFF") == gw2::PlayerNameResult::BadCharacter);
  CHECK(gw2::CheckPlayerName("   ") == gw2::PlayerNameResult::Empty);
  CHECK(std::string(gw2::PlayerNameMessage(gw2::PlayerNameResult::Ok)).empty());
  const std::string hint = gw2::kPlayerNameHint;
  CHECK(hint == "Up to 15 characters.");
  CHECK(!Has(hint, "byte") && !Has(hint, "accent"));
  CHECK(!Has(gw2::PlayerNameMessage(gw2::PlayerNameResult::TooManyBytes), "accented"));
}

void TestEditGate() {
  std::printf("the field never holds an oversized or invalid value\n");
  const std::string fifteen = "123456789012345";
  UserDisplayNameResult result;
  CHECK(Edit("12345678901234", fifteen, &result) == fifteen && result == UserDisplayNameResult::Ok);
  CHECK(Edit(fifteen, fifteen + "6", &result) == fifteen && result == UserDisplayNameResult::TooManyCharacters);
  CHECK(Edit(fifteen, "1234567890X12345", &result) == fifteen);
  const std::string accents = Repeat(kAccent, 7) + "a";
  CHECK(Edit(accents, accents, &result) == accents);
  CHECK(Edit(accents, accents + "a", &result) == accents && result == UserDisplayNameResult::TooManyBytes);
  CHECK(Edit(accents, accents + kAccent, &result) == accents && result == UserDisplayNameResult::TooManyBytes);
  CHECK(Edit(Repeat(kAccent, 7), Repeat(kAccent, 8), &result) == Repeat(kAccent, 7) &&
        result == UserDisplayNameResult::TooManyBytes);
  CHECK(Edit("abc", "abc" + std::string(40, 'x'), &result) == "abc");
  CHECK(Edit("abc", "abc" + Repeat(kAccent, 20), &result) == "abc");
  CHECK(Edit("abcdef", "abXYZef") == "abXYZef");
  CHECK(Edit("abcdef", "aXf") == "aXf");
  CHECK(Edit(fifteen, fifteen.substr(0, 14)) == fifteen.substr(0, 14));
  CHECK(Edit(fifteen, fifteen.substr(1)) == fifteen.substr(1));
  CHECK(Edit(fifteen, fifteen.substr(0, 4) + fifteen.substr(5)) == fifteen.substr(0, 4) + fifteen.substr(5));
  CHECK(Edit(accents, accents.substr(0, accents.size() - 1)) == accents.substr(0, accents.size() - 1));
  CHECK(Edit("abc", "") == "");
  CHECK(Edit("abc", "ab\tc", &result) == "abc" && result == UserDisplayNameResult::BadCharacter);
  CHECK(Edit("abc", "ab\xFF", &result) == "abc" && result == UserDisplayNameResult::BadCharacter);
  CHECK(Edit("abc", "ab\xC3", &result) == "abc");
}

void TestSdkValues() {
  std::printf("SDK values follow logical players 1-4\n");
  CHECK(gw2::kPlayerNames.size() == 4);
  for (int i = 0; i < 4; ++i) {
    const auto& player = gw2::kPlayerNames[i];
    CHECK(std::string(player.key) == "gw2_player_" + std::to_string(i + 1) + "_name");
    CHECK(std::string(player.sdk_key) == "local_player_" + std::to_string(i + 1) + "_name");
    CHECK(std::string(player.default_name) == "Player " + std::to_string(i + 1));
  }
  const auto& p1 = gw2::kPlayerNames[0];
  const auto& p2 = gw2::kPlayerNames[1];
  const auto& p3 = gw2::kPlayerNames[2];
  const auto& p4 = gw2::kPlayerNames[3];
  CHECK(p1.send_default && !p2.send_default && !p3.send_default && !p4.send_default);
  CHECK(gw2::SdkPlayerName(p1, "Saulo") == "Saulo");
  CHECK(gw2::SdkPlayerName(p2, "Guest2") == "Guest2");
  CHECK(gw2::SdkPlayerName(p3, "Guest3") == "Guest3");
  CHECK(gw2::SdkPlayerName(p4, "Guest4") == "Guest4");
  CHECK(gw2::SdkPlayerName(p1, "  Saulo ") == "Saulo");
  CHECK(gw2::SdkPlayerName(p1, "Player 1") == "Player 1");
  CHECK(gw2::SdkPlayerName(p1, "") == "Player 1");
  CHECK(gw2::SdkPlayerName(p1, "\xFF") == "Player 1");
  CHECK(gw2::SdkPlayerName(p1, std::string(16, 'a')) == "Player 1");
  CHECK(gw2::SdkPlayerName(p1, Repeat(kAccent, 8)) == "Player 1");
  CHECK(gw2::SdkPlayerName(p2, "Player 2").empty());
  CHECK(gw2::SdkPlayerName(p3, "Player 3").empty());
  CHECK(gw2::SdkPlayerName(p4, "Player 4").empty());
  CHECK(gw2::SdkPlayerName(p2, std::string(16, 'a')).empty());
  CHECK(gw2::SdkPlayerName(p2, "\xFF").empty());
  CHECK(gw2::SdkPlayerName(p2, "a\tb").empty());
  CHECK(gw2::SdkPlayerName(p1, std::string(15, 'a')) == std::string(15, 'a'));
}

void TestSettingsAndLauncher() {
  std::printf("four settings, saved defensively, one section on the Gameplay page, none on Controls\n");
  const std::string cvars = ReadSource("launcher_cvars.cpp");
  CHECK(Has(cvars, "REXCVAR_DEFINE_STRING(gw2_player_1_name, \"Player 1\""));
  CHECK(Has(cvars, "REXCVAR_DEFINE_STRING(gw2_player_2_name, \"Player 2\""));
  CHECK(Has(cvars, "REXCVAR_DEFINE_STRING(gw2_player_3_name, \"Player 3\""));
  CHECK(Has(cvars, "REXCVAR_DEFINE_STRING(gw2_player_4_name, \"Player 4\""));
  CHECK(!Has(cvars, "hcu_") && !Has(cvars, "ssf2t_"));

  const std::string settings = ReadSource("launcher_settings.cpp");
  CHECK(Has(settings, "!IsValidPlayerName(name->second)"));
  CHECK(Has(settings, "name->second = player.default_name;"));
  CHECK(Has(settings, "Assign(player.sdk_key, SdkPlayerName("));
  CHECK(Has(settings, "ApplyPlayerNames(cvars_);"));

  const std::string launcher = ReadSource("launcher.cpp");
  CHECK(Count(launcher, "ui.Section(\"Player names\"") == 1);
  const size_t gameplay = launcher.find("PageGameplay(trg::Ui");
  const size_t controls = launcher.find("PageControls(trg::Ui");
  const size_t names = launcher.find("ui.Section(\"Player names\"");
  CHECK(gameplay < names);
  CHECK(controls > names || controls == std::string::npos);
  CHECK(launcher.find("ui.Section(\"Player names\"", controls) == std::string::npos);
  CHECK(Has(launcher, "ui.Help(kPlayerNamesHelp);"));
  CHECK(std::string(gw2::kPlayerNamesHelp) == "Changes the name shown in-game. Save data is not affected.");
  CHECK(Has(launcher, "ImGuiInputTextFlags_CallbackEdit"));
  CHECK(Has(launcher, "AcceptUserDisplayNameEdit"));
  CHECK(!Has(launcher, "hcu_") && !Has(launcher, "ssf2t_"));

  CHECK(Has(ReadSource("geometry_wars_retro_evolved_2_app.h"), "gw2::ApplyPlayerNames(cvars_);"));
}

void TestIdentityUnchanged() {
  std::printf("renaming never reaches the XUID or the content directory\n");
  const std::string patch = ReadSdkPatch();
  CHECK(Has(patch, "+  const std::string& profile_id() const { return name_; }"));
  CHECK(Has(patch, "-  auto user_name = rex::to_path(kernel_state_->user_profile()->name());"));
  CHECK(Has(patch, "+  auto user_name = rex::to_path(kernel_state_->user_profile()->profile_id());"));
  CHECK(!Has(patch, "xuid_ ="));
  CHECK(!Has(patch, "+  name_ ="));
  CHECK(Has(patch, "+  *name = name_;"));
  CHECK(Has(patch, "user_profile->display_name(user_index, &name)"));
  CHECK(!Has(patch, "+  info->signin_state") && !Has(patch, "-  info->signin_state"));
  CHECK(!Has(patch, "XamUserGetSigninState"));
  CHECK(Has(patch, "local_player_1_name") && Has(patch, "local_player_2_name") &&
        Has(patch, "local_player_3_name") && Has(patch, "local_player_4_name"));
  CHECK(!Has(patch, "hcu_") && !Has(patch, "ssf2t_") && !Has(patch, "gw2_"));
}

void TestSharedValidator() {
  std::printf("the project validator is the SDK patch's header, byte for byte\n");
  const std::string patch = ReadSdkPatch();
  const std::string start = "+++ b/include/rex/system/xam/user_display_name.h\n";
  size_t at = patch.find(start);
  CHECK(at != std::string::npos);
  at = patch.find('\n', patch.find("@@", at)) + 1;
  const size_t end = patch.find("\ndiff --git", at);
  std::string added;
  std::istringstream lines(patch.substr(at, end - at));
  for (std::string line; std::getline(lines, line);) {
    if (!line.empty() && line[0] == '+') {
      added += line.substr(1) + "\n";
    }
  }
  std::string project;
  for (char c : ReadSource("user_display_name.h")) {
    if (c != '\r') {
      project += c;
    }
  }
  CHECK(!added.empty() && added == project);
}

void TestCachedNameBytes() {
  std::printf("the per-slot display-name cache is only re-stamped when a custom name is configured\n");
  char buffer[16];
  std::memset(buffer, 0x7F, sizeof buffer);
  CHECK(!gw2::CachedPlayerNameBytes("", buffer));
  CHECK(gw2::CachedPlayerNameBytes("Guest2", buffer));
  CHECK(std::string(buffer) == "Guest2");
  CHECK(buffer[6] == '\0' && buffer[15] == '\0');
  const std::string fifteen(15, 'a');
  CHECK(gw2::CachedPlayerNameBytes(fifteen, buffer));
  CHECK(std::string(buffer, 15) == fifteen);
  CHECK(buffer[15] == '\0');
  const std::string accents = Repeat(kAccent, 7) + "a";
  CHECK(gw2::CachedPlayerNameBytes(accents, buffer));
  CHECK(std::string(buffer) == accents);
}

void TestMappingAndRegression() {
  std::printf("logical mapping, controller/input and defaults are left alone\n");
  const std::string names = ReadSource("player_names.h");
  CHECK(!Has(names, "controller") && !Has(names, "xuid"));
  CHECK(!Has(names, "SigninState"));
  CHECK(!Has(ReadSdkPatch(), "SigninState"));
}

void TestDisplayHook() {
  std::printf("the display-name hook targets the real Multiplayer-panel refresh, not the unrelated profile cache\n");
  const std::string hook = ReadSource("player_name_display_hook.cpp");
  CHECK(Has(hook, "REX_EXTERN(__imp__sub_8207DD78);"));
  CHECK(Has(hook, "REX_HOOK_RAW(sub_8207DD78)"));
  CHECK(Has(hook, "__imp__sub_8207DD78(ctx, base);"));
  CHECK(Has(hook, "CachedPlayerNameBytes"));
  CHECK(Has(hook, "PlayerRecordNameAddress"));
  CHECK(!Has(hook, "sub_8208BA90"));
  CHECK(!Has(hook, "SigninState"));
  CHECK(!Has(hook, "r3.u32 = "));  // never fabricates a return value for the original function
}

void TestPlayerRecordAddresses() {
  std::printf("the four player records are 104 bytes apart, matching the game's own record-advance loop\n");
  CHECK(gw2::PlayerRecordNameAddress(0, 1) == 0x821D380Cu);
  CHECK(gw2::PlayerRecordNameAddress(1, 1) == 0x821D3874u);
  CHECK(gw2::PlayerRecordNameAddress(2, 1) == 0x821D38DCu);
  CHECK(gw2::PlayerRecordNameAddress(3, 1) == 0x821D3944u);
  for (uint32_t i = 0; i < 3; ++i) {
    CHECK(gw2::PlayerRecordNameAddress(i + 1, 0) - gw2::PlayerRecordNameAddress(i, 0) == gw2::kPlayerRecordStride);
    CHECK(gw2::PlayerRecordNameAddress(i, 1) - gw2::PlayerRecordNameAddress(i, 0) == 16);
  }
}

}  // namespace

int main() {
  TestValidator();
  TestMessages();
  TestEditGate();
  TestSdkValues();
  TestSettingsAndLauncher();
  TestIdentityUnchanged();
  TestSharedValidator();
  TestCachedNameBytes();
  TestMappingAndRegression();
  TestDisplayHook();
  TestPlayerRecordAddresses();
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures ? 1 : 0;
}
