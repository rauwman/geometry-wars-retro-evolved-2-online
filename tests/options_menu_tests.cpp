// Options menu rules: the L3 + R3 combo detector and how a row's value steps through its choices. Logic only; the
// menu itself needs the running runtime and the ImGui drawer.

#include "options_menu_rules.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Check(bool passed, const char* name) {
  if (!passed) {
    ++failures;
    std::printf("FAIL: %s\n", name);
  }
}

}  // namespace

int main() {
  using gw2::ChoiceIndex;
  using gw2::kOptionsMenuButtons;
  using gw2::kXinputButtonLeftThumb;
  using gw2::kXinputButtonRightThumb;
  using gw2::MaxRowCount;
  using gw2::NextChoiceWrapping;
  using gw2::OptionsComboDetector;
  using gw2::StepChoice;
  using gw2::WrapIndex;

  {
    OptionsComboDetector combo;
    Check(!combo.Poll(0), "neither button down does not press");
    Check(!combo.Poll(kXinputButtonLeftThumb), "only L3 down does not press");
    Check(combo.Poll(kOptionsMenuButtons), "both together presses");
    Check(combo.held(), "held() is true while both are down");
    Check(!combo.Poll(kOptionsMenuButtons), "held combo presses only once");
    Check(!combo.Poll(kOptionsMenuButtons | 0x1000), "another button held alongside does not press again");
    Check(!combo.Poll(kXinputButtonRightThumb), "releasing one button is not a press");
    Check(!combo.held(), "held() is false once released");
    Check(combo.Poll(kOptionsMenuButtons), "pressing again after a release presses again");
  }

  {
    OptionsComboDetector combo;
    combo.Poll(kOptionsMenuButtons);
    Check(!combo.Poll(0), "releasing both at once is not a press");
    Check(combo.Poll(kOptionsMenuButtons | 0x0001 | 0x2000), "unrelated bits set alongside the pair still press");
  }

  {
    const std::vector<std::string> values = {"1", "2", "3", "4"};
    Check(ChoiceIndex(values, "3") == 2, "ChoiceIndex finds the matching value");
    Check(ChoiceIndex(values, "7") == -1, "ChoiceIndex reports no match as -1");
  }

  Check(StepChoice(1, 4, 1) == 2, "StepChoice moves forward one");
  Check(StepChoice(1, 4, -1) == 0, "StepChoice moves back one");
  Check(StepChoice(0, 4, -1) == 0, "StepChoice clamps at the first choice");
  Check(StepChoice(3, 4, 1) == 3, "StepChoice clamps at the last choice");
  Check(StepChoice(-1, 4, 1) == 0, "StepChoice from an unknown value moves to the first choice (forward)");
  Check(StepChoice(-1, 4, -1) == 0, "StepChoice from an unknown value moves to the first choice (back)");
  Check(StepChoice(0, 0, 1) == -1, "StepChoice with no choices is -1");

  Check(NextChoiceWrapping(0, 2) == 1, "NextChoiceWrapping moves to the next choice");
  Check(NextChoiceWrapping(1, 2) == 0, "NextChoiceWrapping wraps from the last choice to the first");
  Check(NextChoiceWrapping(-1, 2) == 0, "NextChoiceWrapping from an unknown value goes to the first choice");
  Check(NextChoiceWrapping(0, 0) == -1, "NextChoiceWrapping with no choices is -1");

  Check(WrapIndex(0, 5, -1) == 4, "WrapIndex wraps backward from the first page");
  Check(WrapIndex(4, 5, 1) == 0, "WrapIndex wraps forward from the last page");
  Check(WrapIndex(2, 5, 1) == 3, "WrapIndex moves forward in the middle");
  Check(WrapIndex(0, 0, 1) == 0, "WrapIndex with no pages stays at 0");

  Check(MaxRowCount({1, 7, 6, 4, 2, 5}) == 7, "MaxRowCount is the tallest page, independent of which is selected");
  Check(MaxRowCount({3}) == 3, "MaxRowCount with one page is that page's row count");
  Check(MaxRowCount({}) == 0, "MaxRowCount with no pages is 0");

  if (failures) {
    std::printf("OPTIONS MENU RULES: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("OPTIONS MENU RULES OK: 0 failure(s)\n");
  return 0;
}
