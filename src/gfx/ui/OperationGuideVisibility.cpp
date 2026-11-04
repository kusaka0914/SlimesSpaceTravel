#include "gfx/ui/OperationGuideVisibility.h"

bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState)
{
    if (displayState.isPauseMenuOpen && !displayState.isUGCPlaytestActive) {
        return true;
    }
    return elementId == "buttonA_copy" || elementId == "buttonTextB_copy";
}
