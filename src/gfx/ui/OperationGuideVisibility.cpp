#include "gfx/ui/OperationGuideVisibility.h"

namespace {
bool IsDodgeGuide(std::string_view elementId)
{
    return elementId == "buttonB" || elementId == "buttonTextB";
}

bool IsStrongAttackGuide(std::string_view elementId)
{
    return elementId == "buttonX" || elementId == "buttonTextX";
}

bool IsCombatGuide(std::string_view elementId)
{
    return IsStrongAttackGuide(elementId) ||
           elementId == "buttonY" || elementId == "buttonTextY" ||
           elementId == "buttonB_copy_copy2_copy2" ||
           elementId == "buttonTextB_copy2_copy2_copy2";
}
}

bool ShouldShowOperationGuideElement(
    std::string_view elementId,
    const OperationGuideDisplayState& displayState)
{
    const bool usesPhasedTutorialGuide =
        !displayState.isUGCPlaytestActive &&
        displayState.currentStageNumber == 1;
    const bool showsBasicGuidesOnly =
        usesPhasedTutorialGuide &&
        displayState.currentPlanetNumber == 0;
    const bool showsDodgeGuideOnly =
        usesPhasedTutorialGuide &&
        displayState.currentPlanetNumber == 3;
    const bool hasUnlockedDodge = !showsBasicGuidesOnly;
    const bool hasUnlockedCombat =
        !showsBasicGuidesOnly && !showsDodgeGuideOnly;

    if (IsDodgeGuide(elementId)) {
        return hasUnlockedDodge;
    }
    if (!IsCombatGuide(elementId)) {
        return true;
    }
    if (!hasUnlockedCombat) {
        return false;
    }
    return true;
}
