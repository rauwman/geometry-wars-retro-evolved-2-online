#include "add_lives.h"

namespace gw2::add_lives {
namespace {

Requests g_requests;

}  // namespace

void Request() {
  g_requests.Request();
}

uint32_t Take() {
  return g_requests.Take();
}

}  // namespace gw2::add_lives
