#include "debug_bomb.h"

#include <chrono>

namespace gw2::debug_bomb {
namespace {

Requests g_requests;
Grants g_grants;

double Now() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}  // namespace

void RequestBomb() {
  g_requests.Request(Now());
}

uint32_t TakeBombs() {
  const double now = Now();
  g_requests.PlayerUpdated(now);
  return g_requests.Take(now);
}

Grants& GameGrants() {
  return g_grants;
}

}  // namespace gw2::debug_bomb
