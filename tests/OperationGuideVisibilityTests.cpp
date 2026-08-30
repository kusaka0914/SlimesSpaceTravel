#include "TestSupport.h"

#include "gfx/ui/OperationGuideVisibility.h"

#include <functional>
#include <string>
#include <vector>

namespace {
OperationGuideDisplayState AtLocation(
    int stageNumber,
    int planetNumber)
{
    OperationGuideDisplayState displayState;
    displayState.currentStageNumber = stageNumber;
    displayState.currentPlanetNumber = planetNumber;
    return displayState;
}

void StageOnePlanetZeroShowsOnlyBasicOperationGuides()
{
    const OperationGuideDisplayState displayState =
        AtLocation(1, 0);

    ExpectTrue(
        ShouldShowOperationGuideElement("buttonA", displayState),
        "jump guide on stage 1 planet 0");
    ExpectTrue(
        ShouldShowOperationGuideElement(
            "buttonB_copy_copy2_copy",
            displayState),
        "split guide on stage 1 planet 0");
    ExpectFalse(
        ShouldShowOperationGuideElement("buttonB", displayState),
        "dodge guide on stage 1 planet 0");
    ExpectFalse(
        ShouldShowOperationGuideElement("buttonY", displayState),
        "attack guide on stage 1 planet 0");
}

void StageOnePlanetThreeAddsDodgeGuide()
{
    const OperationGuideDisplayState displayState =
        AtLocation(1, 3);

    ExpectTrue(
        ShouldShowOperationGuideElement("buttonB", displayState),
        "dodge guide on stage 1 planet 3");
    ExpectFalse(
        ShouldShowOperationGuideElement("buttonX", displayState),
        "strong attack guide on stage 1 planet 3");
    ExpectFalse(
        ShouldShowOperationGuideElement(
            "buttonB_copy_copy2_copy2",
            displayState),
        "special guide on stage 1 planet 3");
}

void StageOnePlanetOneAddsCombatGuides()
{
    const OperationGuideDisplayState displayState =
        AtLocation(1, 1);

    ExpectTrue(
        ShouldShowOperationGuideElement("buttonY", displayState),
        "weak attack guide on stage 1 planet 1");
    ExpectTrue(
        ShouldShowOperationGuideElement("buttonX", displayState),
        "strong attack guide on stage 1 planet 1");
    ExpectTrue(
        ShouldShowOperationGuideElement(
            "buttonB_copy_copy2_copy2",
            displayState),
        "special guide on stage 1 planet 1");
}

void OtherLocationsShowAllOperationGuides()
{
    const OperationGuideDisplayState otherTutorialPlanet =
        AtLocation(1, 2);
    ExpectTrue(
        ShouldShowOperationGuideElement("buttonY", otherTutorialPlanet),
        "combat guide on another stage 1 planet");

    const OperationGuideDisplayState base = AtLocation(0, 0);
    ExpectTrue(
        ShouldShowOperationGuideElement("buttonY", base),
        "combat guide in base");

    const OperationGuideDisplayState laterStage = AtLocation(2, 0);
    ExpectTrue(
        ShouldShowOperationGuideElement("buttonY", laterStage),
        "combat guide in stage 2 or later");
}

void UGCPlaytestsShowAllUnlockedActions()
{
    OperationGuideDisplayState ugcPlaytest = AtLocation(1, 0);
    ugcPlaytest.isUGCPlaytestActive = true;
    ExpectTrue(
        ShouldShowOperationGuideElement("buttonY", ugcPlaytest),
        "combat guide in UGC playtest");
}
}

void RegisterOperationGuideVisibilityTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "OperationGuideVisibility.StageOnePlanetZeroShowsBasicGuides",
        StageOnePlanetZeroShowsOnlyBasicOperationGuides);
    tests.emplace_back(
        "OperationGuideVisibility.StageOnePlanetThreeAddsDodge",
        StageOnePlanetThreeAddsDodgeGuide);
    tests.emplace_back(
        "OperationGuideVisibility.StageOnePlanetOneAddsCombat",
        StageOnePlanetOneAddsCombatGuides);
    tests.emplace_back(
        "OperationGuideVisibility.OtherLocationsShowAll",
        OtherLocationsShowAllOperationGuides);
    tests.emplace_back(
        "OperationGuideVisibility.UGCShowsAll",
        UGCPlaytestsShowAllUnlockedActions);
}
