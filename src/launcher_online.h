#pragma once

#include <memory>
#include <string>

#include "online_board.h"
#include "online_client.h"
#include "trg/launcher.h"
#include "trg/platform.h"

namespace gw2 {

class GameSettings;

// The launcher's Online page: the player's name on the global leaderboard and their friends. A free name is enough to
// sign up; the account key is kept in the player settings. Friends need consent: a player is asked by name and has to
// accept. Every network call runs on a worker thread; the page only draws and polls.
class OnlinePage {
 public:
  explicit OnlinePage(GameSettings& settings) : settings_(settings) {}

  void Draw(trg::Ui& ui);
  // Once per frame, whichever page is open, so a finished call is applied.
  void Poll();

 private:
  enum class Kind {
    kCheck,
    kRegister,
    kRename,
    kRestore,
    kFriends,      // load the friends and the requests
    kSendRequest,
    kAnswer,       // accept, decline, cancel or remove
    kDelete,
  };

  struct Outcome {
    Kind kind = Kind::kCheck;
    std::string name;  // the name the call was about
    online::NameCheck check;
    online::AccountError error = online::AccountError::kNone;
    std::string message;
    online::Account account;
    std::string url;  // the normalized address the call used
    online::FriendsOverview overview;
    online::FriendAction action = online::FriendAction::kAccept;
    std::string xuid;  // the player an answer was about
    bool friends_now = false;
  };

  void LoadBuffers();
  std::string ServerUrl() const;
  void StartCheck();
  void StartRegister();
  void StartRename();
  void StartRestore();
  void StartFriends();
  void StartSendRequest();
  void StartAnswer(online::FriendAction action, const online::Person& person);
  void StartDelete();
  void SignOut();
  void Say(std::string text, bool good);
  void DrawFriends(trg::Ui& ui, bool busy);

  GameSettings& settings_;
  bool loaded_ = false;
  char name_[64] = {};
  char key_[256] = {};
  char friend_name_[64] = {};

  double edited_at_ = 0.0;
  std::string checked_name_;  // the name and server the last finished check was about
  std::string checked_url_;
  online::NameCheck check_;

  online::FriendsOverview overview_;
  bool overview_loaded_ = false;
  bool overview_stale_ = true;       // load again as soon as no other call runs
  double overview_asked_at_ = -1e9;  // when the page last asked, to reload now and then while it is open
  std::string overview_error_;
  std::string confirm_remove_;       // the xuid of the friend whose removal waits for a second click

  std::string message_;
  bool message_good_ = false;

  trg::Task task_;
  std::shared_ptr<Outcome> outcome_;
};

}  // namespace gw2
