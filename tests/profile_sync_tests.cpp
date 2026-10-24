// Tests of sharing the game's progress between profile folders (one folder per online name).
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "profile_sync.h"

namespace fs = std::filesystem;

static int g_failures = 0;
static void Check(bool ok, const std::string& what) {
  std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}

static void Write(const fs::path& file, const std::string& text, int age_minutes) {
  fs::create_directories(file.parent_path());
  {
    std::ofstream out(file, std::ios::binary);
    out << text;
  }
  fs::last_write_time(file, fs::file_time_type::clock::now() - std::chrono::minutes(age_minutes));
}

static std::string Read(const fs::path& file) {
  std::ifstream in(file, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main() {
  const fs::path root = fs::temp_directory_path() / "gw2_profile_sync_tests";
  fs::remove_all(root);

  {
    Check(gw2::SharePlayerProgress(root / "missing", "Yann") == 0, "no profile folder, nothing to do");
    fs::create_directories(root / "a");
    Check(gw2::SharePlayerProgress(root / "a", "Yann") == 0, "an empty profile folder, nothing to do");
    Check(gw2::SharePlayerProgress(root / "a", "") == 0, "no name, nothing to do");
  }

  {
    const fs::path p = root / "new_name";
    Write(p / "User" / "63E83FFF", "progress", 10);
    Check(gw2::SharePlayerProgress(p, "Yann") == 1, "a new name receives the progress");
    Check(Read(p / "Yann" / "63E83FFF") == "progress", "and it is the same content");
    Check(Read(p / "User" / "63E83FFF") == "progress", "the source stays");
    Check(gw2::SharePlayerProgress(p, "Yann") == 0, "asked again, nothing is copied");
  }

  {
    const fs::path p = root / "newest";
    Write(p / "User" / "63E83FFF", "old", 60);
    Write(p / "Yann" / "63E83FFF", "newer", 5);
    Check(gw2::SharePlayerProgress(p, "Rauwman") == 1, "a third name gets one file");
    Check(Read(p / "Rauwman" / "63E83FFF") == "newer", "the newest copy wins");
    Check(Read(p / "User" / "63E83FFF") == "old", "other folders are not touched");
  }

  {
    const fs::path p = root / "active_newer";
    Write(p / "User" / "63E83FFF", "old", 60);
    Write(p / "Yann" / "63E83FFF", "played today", 1);
    Check(gw2::SharePlayerProgress(p, "Yann") == 0, "the active folder is newer: left alone");
    Check(Read(p / "Yann" / "63E83FFF") == "played today", "nothing overwritten");
  }

  {
    const fs::path p = root / "active_older";
    Write(p / "User" / "63E83FFF", "from the other name", 5);
    Write(p / "Yann" / "63E83FFF", "stale", 120);
    Check(gw2::SharePlayerProgress(p, "Yann") == 1, "the active folder is older: updated");
    Check(Read(p / "Yann" / "63E83FFF") == "from the other name", "with the newer content");
  }

  {
    const fs::path p = root / "several";
    Write(p / "User" / "63E83FFF", "a", 10);
    Write(p / "User" / "00000001", "b", 10);
    Write(p / "User" / "temp.tmp", "x", 10);
    Check(gw2::SharePlayerProgress(p, "Yann") == 2, "every file is shared, leftover .tmp files are not");
    Check(!fs::exists(p / "Yann" / "temp.tmp"), "no .tmp file copied");
  }

  fs::remove_all(root);
  std::printf("\n%s: %d failure(s)\n", g_failures ? "PROFILE SYNC TESTS FAILED" : "PROFILE SYNC TESTS OK", g_failures);
  return g_failures ? 1 : 0;
}
