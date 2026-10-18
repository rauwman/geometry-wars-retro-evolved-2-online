#pragma once

#include <cstdint>
#include <string>

namespace gw2 {

// The first data members of ReXGlue 0.10.0's rex::system::xam::UserProfile (xuid_, then name_), which has no setter.
// The game reads the name through XamUserGetName, XamUserGetGamerTag and XUserGetInfo, and the file names of the saves
// do not use it, so replacing it changes what the game shows and nothing else.
struct ProfileLayout {
  uint64_t xuid;
  std::string name;
};

// Replaces the profile's name. Checked before anything is written: the mirror must read back the profile's own xuid
// and name, and the new name must fit the string's current buffer, so nothing is ever allocated or freed here (the
// runtime library owns that memory). Returns false and changes nothing otherwise.
template <typename Profile>
bool SetProfileName(const Profile* profile, const std::string& name) {
  if (!profile || name.empty() || name.size() > 15) return false;
  auto* layout = reinterpret_cast<ProfileLayout*>(const_cast<Profile*>(profile));
  if (layout->xuid != profile->xuid() || layout->name != profile->name()) return false;
  if (layout->name.capacity() < name.size()) return false;
  layout->name.assign(name);
  return profile->name() == name;
}

}  // namespace gw2
