#include "gfx/ui/OperationGuideVisibility.h"

namespace {
bool IsSoloPlayerConfigurationGuide(std::string_view elementId)
{
    return elementId == "buttonB_copy_copy2" || elementId == "buttonTextB_copy2_copy2" ||
           elementId == "buttonB_copy_copy2_copy" || elementId == "buttonTextB_copy2_copy2_copy";
}
} // namespace

bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState)
{
    if (!displayState.isUGCPlaytestActive) {
        if (displayState.isPauseMenuOpen) {
            return true;
        }
        return elementId == "buttonA_copy" || elementId == "buttonTextB_copy";
    }
    return !displayState.isTwoPlayerMode || !IsSoloPlayerConfigurationGuide(elementId);
}
