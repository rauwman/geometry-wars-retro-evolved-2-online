#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "live_stats.h"

namespace gw2::live {

// The app side of what the game asks the host for. Both run on a guest thread.
struct AppBridge {
  std::function<void()> open_achievements;
  std::function<void()> close_achievements;
  // L3 + R3 pressed together on a controller the game reads: opens or closes the in-game Options menu.
  std::function<void()> toggle_options;
};

struct InstallParams {
  std::filesystem::path stats_file;
  LocalUser user;
  AppBridge bridge;
};

// Called once, before the game starts.
void Install(InstallParams params);

// The player's online account, read from the settings once the launcher is closed. A name shows in the game's
// leaderboards; with a server address and token, clean scores also go to the server in the background. Missing parts
// leave the game offline, as before. Call once, before the game starts.
struct OnlineParams {
  std::string url;
  std::string token;
  std::string name;
};
void ConfigureOnline(const OnlineParams& params);

// Set while the achievements overlay is up: the game's controller input stays neutral, and B closes
// the overlay without the game seeing that press.
void SetAchievementsModal(bool modal);

// Whether the achievements gate needs the game's input unfiltered: the overlay is up, or the B that closed it is still
// held. Safe to read from any thread.
bool AchievementsHoldInput();

// Left stick or D-pad direction read from the game's controller while the overlay is up: -1 up .. 1 down.
float AchievementsScrollAxis();

}  // namespace gw2::live
