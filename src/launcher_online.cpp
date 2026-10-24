#include "launcher_online.h"

#include <cfloat>
#include <cstdio>
#include <utility>

#include <imgui.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif

#include <rex/logging.h>

#include "launcher_settings.h"

namespace gw2 {
namespace {

constexpr double kCheckDelaySeconds = 0.4;
constexpr double kReloadFriendsSeconds = 20.0;

const ImVec4 kGood(0.35f, 0.85f, 0.45f, 1.0f);
const ImVec4 kBad(0.95f, 0.40f, 0.35f, 1.0f);

void CopyInto(char* out, size_t size, const std::string& text) {
  std::snprintf(out, size, "%s", text.c_str());
}

// The clipboard's text as one trimmed line of UTF-8, or an empty string. The launcher's text fields do not always
// receive Ctrl+V, so the page has Paste buttons that read the clipboard themselves.
std::string ClipboardLine() {
  std::string text;
#if defined(_WIN32)
  if (OpenClipboard(nullptr)) {
    if (const HANDLE handle = GetClipboardData(CF_UNICODETEXT)) {
      if (const auto* wide = static_cast<const wchar_t*>(GlobalLock(handle))) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (size > 1) {
          text.resize(static_cast<size_t>(size));
          WideCharToMultiByte(CP_UTF8, 0, wide, -1, text.data(), size, nullptr, nullptr);
          text.resize(static_cast<size_t>(size) - 1);
        }
        GlobalUnlock(handle);
      }
    }
    CloseClipboard();
  }
#else
  if (char* clip = SDL_GetClipboardText()) {
    text = clip;
    SDL_free(clip);
  }
#endif
  const size_t end_of_line = text.find_first_of("\r\n");
  if (end_of_line != std::string::npos) text.resize(end_of_line);
  constexpr const char* kSpace = " \t\"'";
  const size_t first = text.find_first_not_of(kSpace);
  if (first == std::string::npos) return {};
  return text.substr(first, text.find_last_not_of(kSpace) - first + 1);
}

// Puts text on the system clipboard. False when that fails.
bool SetClipboardText(const std::string& text) {
#if defined(_WIN32)
  const int wide_size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (wide_size <= 0) return false;
  bool done = false;
  if (OpenClipboard(nullptr)) {
    if (const HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wide_size) * sizeof(wchar_t))) {
      if (auto* wide = static_cast<wchar_t*>(GlobalLock(memory))) {
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide, wide_size);
        GlobalUnlock(memory);
        EmptyClipboard();
        done = SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
      }
      if (!done) GlobalFree(memory);
    }
    CloseClipboard();
  }
  return done;
#else
  return SDL_SetClipboardText(text.c_str());
#endif
}

// A text field with a Paste button beside it. Returns 1 when the field was edited, 2 when text was pasted.
int PasteField(const char* id, const char* hint, char* buffer, size_t size, bool disabled, int flags = 0) {
  const ImGuiStyle& style = ImGui::GetStyle();
  const float button_width = ImGui::CalcTextSize("Paste").x + style.FramePadding.x * 2.0f;
  int result = 0;
  ImGui::PushID(id);
  ImGui::SetNextItemWidth(-(button_width + style.ItemSpacing.x));
  if (ImGui::InputTextWithHint("##field", hint, buffer, size, flags)) result = 1;
  if (ImGui::IsItemDeactivatedAfterEdit()) result = 1;
  ImGui::SameLine();
  ImGui::BeginDisabled(disabled);
  if (ImGui::Button("Paste", ImVec2(button_width, 0.0f))) {
    const std::string text = ClipboardLine();
    if (!text.empty()) {
      CopyInto(buffer, size, text);
      result = 2;
    }
  }
  ImGui::EndDisabled();
  ImGui::PopID();
  return result;
}

}  // namespace

void OnlinePage::LoadBuffers() {
  if (loaded_) return;
  loaded_ = true;
  CopyInto(name_, sizeof name_, settings_.Get(kOnlineNameKey));
}

// The server comes with the game. A different one can be set in the player settings (gw2_online_url) for testing.
std::string OnlinePage::ServerUrl() const {
  return online::NormalizeBaseUrl(settings_.Get(kOnlineUrlKey, kDefaultOnlineUrl));
}

void OnlinePage::Say(std::string text, bool good) {
  message_ = std::move(text);
  message_good_ = good;
}

void OnlinePage::StartCheck() {
  const std::string url = ServerUrl();
  const std::string name = name_;
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kCheck;
  outcome->name = name;
  outcome->url = url;
  const bool started = task_.Start("Checking name...", [outcome, url, name](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    outcome->check = online::CheckName(*transport, url, name);
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartRegister() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kRegister;
  outcome->name = name_;
  outcome->url = ServerUrl();
  Say("Creating your name...", true);
  const bool started = task_.Start("Creating online name...", [outcome](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::RegisterResult result = online::Register(*transport, outcome->url, outcome->name);
    outcome->error = result.error;
    outcome->message = result.message;
    outcome->account = result.account;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartRestore() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kRestore;
  outcome->url = ServerUrl();
  const std::string key = key_;
  Say("Restoring your account...", true);
  const bool started = task_.Start("Restoring online account...", [outcome, key](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::RegisterResult result = online::Restore(*transport, outcome->url, key);
    outcome->error = result.error;
    outcome->message = result.message;
    outcome->account = result.account;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartRename() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kRename;
  outcome->name = name_;
  outcome->url = ServerUrl();
  const std::string token = settings_.Get(kOnlineTokenKey);
  Say("Changing your name...", true);
  const bool started = task_.Start("Changing online name...", [outcome, token](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::RenameResult result = online::Rename(*transport, outcome->url, token, outcome->name);
    outcome->error = result.error;
    outcome->message = result.message;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartFriends() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kFriends;
  outcome->url = ServerUrl();
  const std::string token = settings_.Get(kOnlineTokenKey);
  const bool started = task_.Start("Loading friends...", [outcome, token](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::FriendsResult result = online::FetchFriends(*transport, outcome->url, token);
    outcome->error = result.error;
    outcome->message = result.message;
    outcome->overview = result.overview;
    return std::string();
  });
  if (started) {
    outcome_ = std::move(outcome);
    overview_stale_ = false;
    overview_asked_at_ = ImGui::GetTime();
  }
}

void OnlinePage::StartSendRequest() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kSendRequest;
  outcome->url = ServerUrl();
  outcome->name = friend_name_;
  const std::string token = settings_.Get(kOnlineTokenKey);
  Say("Sending the request...", true);
  const bool started = task_.Start("Sending friend request...", [outcome, token](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::FriendRequestResult result =
        online::SendFriendRequest(*transport, outcome->url, token, outcome->name);
    outcome->error = result.error;
    outcome->message = result.message;
    outcome->name = result.person.name;
    outcome->friends_now = result.friends;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartAnswer(online::FriendAction action, const online::Person& person) {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kAnswer;
  outcome->url = ServerUrl();
  outcome->action = action;
  outcome->xuid = person.xuid;
  outcome->name = person.name;
  const std::string token = settings_.Get(kOnlineTokenKey);
  confirm_remove_.clear();
  const bool started = task_.Start("Updating friends...", [outcome, token](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::ActionResult result =
        online::AnswerFriend(*transport, outcome->url, token, outcome->action, outcome->xuid);
    outcome->error = result.error;
    outcome->message = result.message;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::StartDelete() {
  auto outcome = std::make_shared<Outcome>();
  outcome->kind = Kind::kDelete;
  outcome->url = ServerUrl();
  const std::string token = settings_.Get(kOnlineTokenKey);
  Say("Deleting your account...", true);
  const bool started = task_.Start("Deleting online account...", [outcome, token](trg::Task&) {
    const auto transport = online::MakeSystemTransport();
    const online::ActionResult result = online::DeleteAccount(*transport, outcome->url, token);
    outcome->error = result.error;
    outcome->message = result.message;
    return std::string();
  });
  if (started) outcome_ = std::move(outcome);
}

void OnlinePage::SignOut() {
  if (settings_.SaveNow({{kOnlineNameKey, ""}, {kOnlineTokenKey, ""}})) {
    name_[0] = '\0';
    checked_name_.clear();
    overview_ = {};
    overview_loaded_ = false;
    overview_stale_ = true;
    overview_error_.clear();
    Say("Signed out. Your scores stay on this PC.", true);
  } else {
    Say("Could not save the settings.", false);
  }
}

void OnlinePage::Poll() {
  std::string error;
  if (task_.Finish(&error) == trg::Task::State::kIdle) return;
  const std::shared_ptr<Outcome> outcome = std::move(outcome_);
  if (!outcome) return;

  const auto failure = [&] {
    Say(outcome->message.empty() ? online::DescribeError(outcome->error) : outcome->message, false);
  };
  const bool failed = outcome->error != online::AccountError::kNone;

  switch (outcome->kind) {
    case Kind::kCheck:
      checked_name_ = outcome->name;
      checked_url_ = outcome->url;
      check_ = outcome->check;
      break;
    case Kind::kRegister:
      if (failed) {
        failure();
        break;
      }
      // The address is not saved: it comes with the game unless the player settings name another one.
      if (settings_.SaveNow({{kOnlineNameKey, outcome->account.name}, {kOnlineTokenKey, outcome->account.token}})) {
        CopyInto(name_, sizeof name_, outcome->account.name);
        Say("Welcome, " + outcome->account.name + "! Your scores now go to the leaderboard.", true);
        REXLOG_INFO("Online: created the name \"{}\"", outcome->account.name);
      } else {
        Say("The name was created but could not be saved here. Check that the data folder is writable.", false);
      }
      checked_name_.clear();
      overview_stale_ = true;
      break;
    case Kind::kRestore:
      if (failed) {
        failure();
        break;
      }
      if (settings_.SaveNow({{kOnlineNameKey, outcome->account.name}, {kOnlineTokenKey, outcome->account.token}})) {
        CopyInto(name_, sizeof name_, outcome->account.name);
        key_[0] = '\0';
        Say("Welcome back, " + outcome->account.name + "! Your scores go to the leaderboard again.", true);
        REXLOG_INFO("Online: restored the name \"{}\"", outcome->account.name);
      } else {
        Say("The account was found but could not be saved here. Check that the data folder is writable.", false);
      }
      checked_name_.clear();
      overview_stale_ = true;
      break;
    case Kind::kRename:
      if (failed) {
        failure();
        break;
      }
      if (settings_.SaveNow(kOnlineNameKey, outcome->name)) {
        Say("Your name is now " + outcome->name + ".", true);
      } else {
        Say("The name was changed but could not be saved here.", false);
      }
      checked_name_.clear();
      break;
    case Kind::kFriends:
      if (failed) {
        overview_error_ = outcome->message.empty() ? online::DescribeError(outcome->error) : outcome->message;
        break;
      }
      overview_ = outcome->overview;
      overview_loaded_ = true;
      overview_error_.clear();
      break;
    case Kind::kSendRequest:
      if (failed) {
        failure();
        break;
      }
      friend_name_[0] = '\0';
      if (outcome->friends_now) {
        Say("You and " + outcome->name + " are friends now.", true);
      } else {
        Say("Request sent to " + outcome->name + ". It counts as soon as they accept it.", true);
      }
      overview_stale_ = true;
      break;
    case Kind::kAnswer:
      if (failed) {
        failure();
        overview_stale_ = true;  // the list may be out of date: that is often why it failed
        break;
      }
      switch (outcome->action) {
        case online::FriendAction::kAccept: Say("You and " + outcome->name + " are friends now.", true); break;
        case online::FriendAction::kDecline: Say("Request declined. " + outcome->name + " is not told.", true); break;
        case online::FriendAction::kCancel: Say("Request to " + outcome->name + " taken back.", true); break;
        case online::FriendAction::kRemove: Say(outcome->name + " is no longer your friend.", true); break;
      }
      overview_stale_ = true;
      break;
    case Kind::kDelete:
      if (failed) {
        failure();
        break;
      }
      if (settings_.SaveNow({{kOnlineNameKey, ""}, {kOnlineTokenKey, ""}})) {
        name_[0] = '\0';
        overview_ = {};
        overview_loaded_ = false;
        overview_stale_ = true;
        Say("Your online account is deleted. Your scores stay on this PC.", true);
        REXLOG_INFO("Online: the account was deleted");
      } else {
        Say("The account was deleted on the server, but this PC could not forget the key. Use Sign out.", false);
      }
      checked_name_.clear();
      break;
  }
}

void OnlinePage::DrawFriends(trg::Ui& ui, bool busy) {
  // -- add a friend
  ui.Row("Add a friend", "Enter the name of a player. They get a request and have to accept it.");
  PasteField("friendname", "player name", friend_name_, sizeof friend_name_, false);
  ui.Row("", nullptr);
  const bool can_send = !busy && !online::FriendTarget(friend_name_).empty();
  ImGui::BeginDisabled(!can_send);
  if (ui.AccentButton("Send request")) StartSendRequest();
  ImGui::EndDisabled();

  // -- requests
  if (!overview_.incoming.empty()) {
    ui.Row("Requests", "These players want to be your friend.");
    for (const online::Person& person : overview_.incoming) {
      ImGui::PushID(person.xuid.c_str());
      ImGui::TextUnformatted(person.name.c_str());
      ImGui::SameLine();
      ImGui::BeginDisabled(busy);
      if (ImGui::SmallButton("Accept")) StartAnswer(online::FriendAction::kAccept, person);
      ImGui::SameLine();
      if (ImGui::SmallButton("Decline")) StartAnswer(online::FriendAction::kDecline, person);
      ImGui::EndDisabled();
      ImGui::PopID();
    }
  }
  if (!overview_.outgoing.empty()) {
    ui.Row("Sent requests", "Waiting for an answer.");
    for (const online::Person& person : overview_.outgoing) {
      ImGui::PushID(person.xuid.c_str());
      ImGui::TextUnformatted(person.name.c_str());
      ImGui::SameLine();
      ImGui::BeginDisabled(busy);
      if (ImGui::SmallButton("Take back")) StartAnswer(online::FriendAction::kCancel, person);
      ImGui::EndDisabled();
      ImGui::PopID();
    }
  }

  // -- friends
  ui.Row("Friends", "The Friends board in the game shows them and you. New friends appear when the game starts.");
  if (overview_.friends.empty()) {
    ImGui::TextDisabled("%s", overview_loaded_ ? "No friends yet." : "");
  }
  for (const online::Person& person : overview_.friends) {
    ImGui::PushID(person.xuid.c_str());
    ImGui::TextUnformatted(person.name.c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(busy);
    if (confirm_remove_ == person.xuid) {
      ImGui::TextUnformatted("Remove?");
      ImGui::SameLine();
      if (ImGui::SmallButton("Yes")) StartAnswer(online::FriendAction::kRemove, person);
      ImGui::SameLine();
      if (ImGui::SmallButton("No")) confirm_remove_.clear();
    } else if (ImGui::SmallButton("Remove")) {
      confirm_remove_ = person.xuid;
    }
    ImGui::EndDisabled();
    ImGui::PopID();
  }
  if (!overview_error_.empty()) {
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(kBad, "Could not refresh the friends: %s", overview_error_.c_str());
    ImGui::PopTextWrapPos();
  }
}

void OnlinePage::Draw(trg::Ui& ui) {
  LoadBuffers();
  const std::string account_name = settings_.Get(kOnlineNameKey);
  const bool signed_in = !account_name.empty() && !settings_.Get(kOnlineTokenKey).empty();
  const bool busy = task_.running();
  const std::string url = ServerUrl();

  ui.Help(signed_in ? "You are on the global leaderboard. Your best scores are uploaded when a game ends. Friends are "
                      "players you and they both agreed on."
                    : "Pick a name for the global leaderboard. It is shown in the game and on the website. "
                      "Scores earned with cheats are never uploaded.");
  ui.Spacer(8);

  if (signed_in) {
    // Ask for the friends when the page opens, after every change, and now and then while it stays open.
    if (!busy && (overview_stale_ || ImGui::GetTime() - overview_asked_at_ >= kReloadFriendsSeconds)) StartFriends();
    if (ui.ButtonRow("Account key", "Private. Keep it: with it a reinstalled game gets this name back.",
                     "Copy account key")) {
      Say(SetClipboardText(settings_.Get(kOnlineTokenKey)) ? "Account key copied. Keep it somewhere safe and private."
                                                            : "Could not use the clipboard.",
          true);
    }
  } else {
    overview_stale_ = true;  // load right away after signing in
  }

  // -- name
  ui.Row("Online name", signed_in ? "Change it any time." : "Letters, digits, space, . _ -   Up to 15 characters.");
  if (PasteField("name", "your name", name_, sizeof name_, false) != 0) {
    edited_at_ = ImGui::GetTime();
    message_.clear();
  }

  const std::string typed = name_;
  const bool unchanged = signed_in && typed == account_name;
  std::string status;
  ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
  bool name_ok = false;
  if (typed.empty()) {
    status = signed_in ? "" : "Type the name you want.";
  } else if (unchanged) {
    status = "This is your name.";
  } else if (const std::string problem = online::NameProblem(typed); !problem.empty()) {
    status = problem;
    color = kBad;
  } else if (url.empty()) {
    status = "The server address is not usable. Check gw2_online_url in the player settings.";
  } else if (checked_name_ == typed && checked_url_ == url) {
    switch (check_.state) {
      case online::NameState::kAvailable:
        status = "Free: " + typed + " is yours to take.";
        color = kGood;
        name_ok = true;
        break;
      case online::NameState::kTaken:
        status = typed + " is already taken.";
        color = kBad;
        break;
      case online::NameState::kInvalid:
        status = check_.message;
        color = kBad;
        break;
      case online::NameState::kUnreachable:
        status = check_.message.empty() ? "Could not reach the server." : check_.message;
        color = kBad;
        break;
    }
  } else {
    status = "Checking...";
  }
  // Ask the server once the typing has paused, and again when an answer is about a name no longer typed.
  if (!typed.empty() && !unchanged && !url.empty() && online::NameProblem(typed).empty() &&
      (checked_name_ != typed || checked_url_ != url) && !busy && ImGui::GetTime() - edited_at_ >= kCheckDelaySeconds) {
    StartCheck();
  }
  ui.Row("", nullptr);
  if (!status.empty()) ImGui::TextColored(color, "%s", status.c_str());

  // -- actions
  ui.Row("", nullptr);
  if (!signed_in) {
    const bool can = name_ok && !busy;
    ImGui::BeginDisabled(!can);
    if (ui.AccentButton("Create online name")) StartRegister();
    ImGui::EndDisabled();

    ui.Row("", nullptr);
    ui.Row("Restore", "Name from before a reinstall? Paste your account key.");
    PasteField("key", "account key", key_, sizeof key_, false, ImGuiInputTextFlags_Password);
    ui.Row("", nullptr);
    const bool can_restore = !busy && !url.empty() && key_[0] != '\0';
    ImGui::BeginDisabled(!can_restore);
    if (ui.AccentButton("Restore account")) StartRestore();
    ImGui::EndDisabled();
  } else {
    const bool can = name_ok && !busy && !unchanged;
    ImGui::BeginDisabled(!can);
    if (ui.AccentButton("Change name")) StartRename();
    ImGui::EndDisabled();

    ui.Spacer(8);
    DrawFriends(ui, busy);
    ui.Spacer(8);

    if (ui.ConfirmRow("Sign out", "Stops uploading scores. Your name stays reserved on the server.", "Sign out",
                      "Sign out of the leaderboard on this PC?")) {
      SignOut();
    }
    ImGui::BeginDisabled(busy);
    if (ui.ConfirmRow("Delete account",
                      "Removes your name, scores and friendships from the server for good. Scores on this PC stay.",
                      "Delete account", "Delete your online account for good?")) {
      StartDelete();
    }
    ImGui::EndDisabled();
  }
  if (!message_.empty()) {
    ui.Row("", nullptr);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextColored(message_good_ ? kGood : kBad, "%s", message_.c_str());
    ImGui::PopTextWrapPos();
  }
  ui.EndRows();
}

}  // namespace gw2
