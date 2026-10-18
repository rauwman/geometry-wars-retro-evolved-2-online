// Tests of the profile name replacement against stand-ins for the runtime's UserProfile.
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "profile_name.h"

static int g_failures = 0;
static void Check(bool ok, const std::string& what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}

// The same member order as the runtime's class.
class SameLayout {
 public:
  SameLayout() : xuid_(0xB13EBABEBABEBABEull), name_("User") {}
  uint64_t xuid() const { return xuid_; }
  std::string name() const { return name_; }
  void* AddSetting() { return &settings_; }

 private:
  uint64_t xuid_;
  std::string name_;
  std::vector<int> settings_;
};

// Another layout: the guard must notice.
class OtherLayout {
 public:
  OtherLayout() : name_("User"), xuid_(7) {}
  uint64_t xuid() const { return xuid_; }
  std::string name() const { return name_; }

 private:
  std::string name_;
  uint64_t xuid_;
};

int main() {
  {
    SameLayout profile;
    Check(gw2::SetProfileName(&profile, "Yann R") && profile.name() == "Yann R" && profile.xuid() == 0xB13EBABEBABEBABEull,
          "the name changes and the xuid stays");
    Check(gw2::SetProfileName(&profile, "123456789012345") && profile.name() == "123456789012345", "15 characters fit");
    Check(!gw2::SetProfileName(&profile, "1234567890123456") && profile.name() == "123456789012345",
          "16 characters are refused and nothing changes");
    Check(!gw2::SetProfileName(&profile, "") && profile.name() == "123456789012345", "an empty name is refused");
    Check(!gw2::SetProfileName<SameLayout>(nullptr, "x"), "no profile, no change");
  }
  {
    SameLayout profile;
    auto* wide = new std::string(64, 'x');  // a name too long for the buffer the profile has
    delete wide;
    Check(gw2::SetProfileName(&profile, "A") && profile.name() == "A", "a short name after a long one");
  }
  {
    OtherLayout profile;
    Check(!gw2::SetProfileName(&profile, "Yann R") && profile.name() == "User" && profile.xuid() == 7,
          "a layout that does not match is left alone");
  }
  std::printf("\n%s: %d failure(s)\n", g_failures ? "PROFILE NAME TESTS FAILED" : "PROFILE NAME TESTS OK", g_failures);
  return g_failures ? 1 : 0;
}
