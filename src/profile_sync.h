#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <system_error>

namespace gw2 {

// The runtime keeps the game's profile data (the game's own progress) in <profile root>/<profile name>/<setting id>.
// The online name replaces the profile name, so each name would start with an empty profile: a new account, or a
// renamed one, would have lost its progress. The progress belongs to the installation, not to the name, so before
// the game starts, the active name's folder receives the newest copy of every file found in any folder.
//
// Newest means last write time; a file that is missing or older in the active folder is replaced, a newer one is left
// alone. Returns how many files were copied. Nothing is deleted, and errors skip the file (the game then starts with
// what is there).
inline int SharePlayerProgress(const std::filesystem::path& profile_root, const std::string& active_name) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (active_name.empty() || !fs::is_directory(profile_root, ec)) return 0;

  // Per file name, the newest copy in any other folder.
  struct Newest {
    fs::path path;
    fs::file_time_type time;
  };
  std::map<std::string, Newest> newest;
  for (const auto& folder : fs::directory_iterator(profile_root, ec)) {
    std::error_code item_ec;
    if (!folder.is_directory(item_ec) || folder.path().filename() == active_name) continue;
    for (const auto& file : fs::directory_iterator(folder.path(), item_ec)) {
      std::error_code file_ec;
      if (!file.is_regular_file(file_ec) || file.path().extension() == ".tmp") continue;
      const auto time = fs::last_write_time(file.path(), file_ec);
      if (file_ec) continue;
      const std::string key = file.path().filename().string();
      const auto it = newest.find(key);
      if (it == newest.end() || time > it->second.time) newest[key] = {file.path(), time};
    }
  }
  if (newest.empty()) return 0;

  const fs::path active = profile_root / active_name;
  fs::create_directories(active, ec);
  if (ec) return 0;
  int copied = 0;
  for (const auto& [key, source] : newest) {
    const fs::path target = active / key;
    std::error_code target_ec;
    if (fs::is_regular_file(target, target_ec)) {
      const auto current = fs::last_write_time(target, target_ec);
      if (!target_ec && current >= source.time) continue;
    }
    std::error_code copy_ec;
    if (!fs::copy_file(source.path, target, fs::copy_options::overwrite_existing, copy_ec) || copy_ec) continue;
    fs::last_write_time(target, source.time, copy_ec);  // keep the age, so the next start compares like with like
    ++copied;
  }
  return copied;
}

}  // namespace gw2
