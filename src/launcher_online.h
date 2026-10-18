#pragma once

#include <memory>
#include <string>

#include "online_client.h"
#include "trg/launcher.h"
#include "trg/platform.h"

namespace gw2 {

class GameSettings;

// The launcher's Online page: the player's name on the friends leaderboard. The name is checked against the server
// while typing, created there with the shared invite code, and kept with a private token in the player settings.
// Every network call runs on a worker thread; the page only draws and polls.
class OnlinePage {
 public:
  explicit OnlinePage(GameSettings& settings) : settings_(settings) {}

  void Draw(trg::Ui& ui);
  // Once per frame, whichever page is open, so a finished call is applied.
  void Poll();

 private:
  enum class Kind { kCheck, kRegister, kRename, kRestore };

  struct Outcome {
    Kind kind = Kind::kCheck;
    std::string name;  // the name the call was about
    online::NameCheck check;
    online::AccountError error = online::AccountError::kNone;
    std::string message;
    online::Account account;
    std::string url;     // the normalized address the call used
    std::string invite;
  };

  void LoadBuffers();
  std::string ServerUrl() const;
  void StartCheck();
  void StartRegister();
  void StartRename();
  void StartRestore();
  void SignOut();
  void Say(std::string text, bool good);

  GameSettings& settings_;
  bool loaded_ = false;
  char name_[64] = {};
  char url_[512] = {};
  char invite_[128] = {};
  char key_[256] = {};

  double edited_at_ = 0.0;
  std::string checked_name_;  // the name and server the last finished check was about
  std::string checked_url_;
  online::NameCheck check_;

  std::string message_;
  bool message_good_ = false;

  trg::Task task_;
  std::shared_ptr<Outcome> outcome_;
};

}  // namespace gw2
