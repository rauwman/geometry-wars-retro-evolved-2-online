#pragma once

#include <cstdint>
#include <vector>

#include <SDL3/SDL_gamepad.h>

struct ImGuiIO;
enum ImGuiKey : int;

namespace gw2 {

// Feeds every connected SDL controller to Dear ImGui navigation. Release() lets go of every key, so nothing read here
// stays held in the game.
class ImGuiGamepad {
 public:
  ImGuiGamepad() = default;
  ~ImGuiGamepad();
  ImGuiGamepad(const ImGuiGamepad&) = delete;
  ImGuiGamepad& operator=(const ImGuiGamepad&) = delete;

  void Update(ImGuiIO& io);
  void Release(ImGuiIO& io);

  // The union of every connected pad as of the last Update, read synchronously: ImGui's own IsKeyDown only sees it
  // a frame later, since AddKeyEvent queues it for the next NewFrame.
  bool Held(ImGuiKey key) const;
  bool AnyConnected() const { return !pads_.empty(); }

 private:
  void DrainEvents();
  void CloseAll();

  std::vector<SDL_Gamepad*> pads_;
  uint32_t held_ = 0;  // bit i: kButtons[i] is down
  bool subsystem_ = false;
};

}  // namespace gw2
