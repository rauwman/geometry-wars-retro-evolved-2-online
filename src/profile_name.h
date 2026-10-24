#pragma once

#include <cstdint>
#include <string>

namespace gw2 {

// The first data members of ReXGlue 0.10.0's rex::system::xam::UserProfile (xuid_, then name_), which has no setter.
// The runtime names the folder of the profile data after it (profile/<name>/...; patch 0054 calls it profile_id()),
// so a changed name needs profile_sync.h to carry the progress over. The name the game shows for player 1 comes from
// the runtime's local_player_1_name setting instead (patch 0054), which the app sets to the online name as well.
struct ProfileLayout {
  uint64_t xuid;
  std::string name;
};

// Replaces the profile's name. Checked before anything is written: the mirror must read back the profile's own xuid
// and name, and the new name must fit the string's current buffer, so nothing is ever allocated or freed here (the
// runtime library owns that memory). Returns false and changes nothing otherwise.
// The name the runtime keeps: profile_id() since patch 0054, name() before.
template <typename Profile>
std::string StoredProfileName(const Profile* profile) {
  if constexpr (requires { profile->profile_id(); }) {
    return profile->profile_id();
  } else {
    return profile->name();
  }
}

template <typename Profile>
bool SetProfileName(const Profile* profile, const std::string& name) {
  if (!profile || name.empty() || name.size() > 15) return false;
  auto* layout = reinterpret_cast<ProfileLayout*>(const_cast<Profile*>(profile));
  if (layout->xuid != profile->xuid() || layout->name != StoredProfileName(profile)) return false;
  if (layout->name.capacity() < name.size()) return false;
  layout->name.assign(name);
  return StoredProfileName(profile) == name;
}

}  // namespace gw2
