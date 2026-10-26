#include "support/TestSupport.h"

#include "gfx/ui/ContextualOperationGuide.h"
#include "gfx/ui/OperationGuideVisibility.h"

namespace {
OperationGuideDisplayState AtLocation(int stageNumber, int planetNumber)
{
    OperationGuideDisplayState displayState;
    displayState.currentStageNumber = stageNumber;
    displayState.currentPlanetNumber = planetNumber;
    return displayState;
}

void StageOnePlanetZeroShowsOnlyBasicOperationGuides()
{
    const OperationGuideDisplayState displayState = AtLocation(1, 0);

    ExpectTrue(ShouldShowOperationGuideElement("buttonA", displayState), "jump guide on stage 1 planet 0");
    ExpectTrue(ShouldShowOperationGuideElement("buttonB_copy_copy2_copy", displayState),
               "split guide on stage 1 planet 0");
    ExpectFalse(ShouldShowOperationGuideElement("buttonB", displayState), "dodge guide on stage 1 planet 0");
    ExpectFalse(ShouldShowOperationGuideElement("buttonY", displayState), "attack guide on stage 1 planet 0");
    ExpectFalse(ShouldShowOperationGuideElement("weakAttackHoldGuide", displayState),
                "long Y is hidden before combat tutorial");
    ExpectFalse(ShouldShowOperationGuideElement("strongAttackHoldGuide", displayState),
                "long X is hidden before combat tutorial");
}

void StageOnePlanetThreeAddsDodgeGuide()
{
    const OperationGuideDisplayState displayState = AtLocation(1, 3);

    ExpectTrue(ShouldShowOperationGuideElement("buttonB", displayState), "dodge guide on stage 1 planet 3");
    ExpectFalse(ShouldShowOperationGuideElement("buttonX", displayState), "strong attack guide on stage 1 planet 3");
    ExpectFalse(ShouldShowOperationGuideElement("buttonB_copy_copy2_copy2", displayState),
                "special guide on stage 1 planet 3");
}

void StageOnePlanetOneAddsCombatGuides()
{
    const OperationGuideDisplayState displayState = AtLocation(1, 1);

    ExpectTrue(ShouldShowOperationGuideElement("buttonY", displayState), "weak attack guide on stage 1 planet 1");
    ExpectTrue(ShouldShowOperationGuideElement("weakAttackHoldGuide", displayState),
               "long Y is shown with combat guide");
    ExpectTrue(ShouldShowOperationGuideElement("strongAttackHoldGuide", displayState),
               "long X is shown with combat guide");
    ExpectTrue(ShouldShowOperationGuideElement("buttonX", displayState), "strong attack guide on stage 1 planet 1");
    ExpectTrue(ShouldShowOperationGuideElement("buttonB_copy_copy2_copy2", displayState),
               "special guide on stage 1 planet 1");
}

void OtherLocationsShowAllOperationGuides()
{
    const OperationGuideDisplayState otherTutorialPlanet = AtLocation(1, 2);
    ExpectTrue(ShouldShowOperationGuideElement("buttonY", otherTutorialPlanet),
               "combat guide on another stage 1 planet");

    const OperationGuideDisplayState base = AtLocation(0, 0);
    ExpectTrue(ShouldShowOperationGuideElement("buttonY", base), "combat guide in base");

    const OperationGuideDisplayState laterStage = AtLocation(2, 0);
    ExpectFalse(ShouldShowOperationGuideElement("buttonY", laterStage), "combat guide hidden in stage 2 or later");
    ExpectTrue(ShouldShowOperationGuideElement("buttonA_copy", laterStage),
               "menu icon remains visible in stage 2 or later");
    ExpectTrue(ShouldShowOperationGuideElement("buttonTextB_copy", laterStage),
               "menu label remains visible in stage 2 or later");
}

void UGCPlaytestsShowAllUnlockedActions()
{
    OperationGuideDisplayState ugcPlaytest = AtLocation(1, 0);
    ugcPlaytest.isUGCPlaytestActive = true;
    ExpectTrue(ShouldShowOperationGuideElement("buttonY", ugcPlaytest), "combat guide in UGC playtest");
}

void TwoPlayerModeHidesSoloConfigurationGuides()
{
    OperationGuideDisplayState displayState = AtLocation(1, 1);
    displayState.isTwoPlayerMode = true;

    ExpectFalse(ShouldShowOperationGuideElement("buttonB_copy_copy2", displayState),
                "player switch button in two player mode");
    ExpectFalse(ShouldShowOperationGuideElement("buttonTextB_copy2_copy2", displayState),
                "player switch text in two player mode");
    ExpectFalse(ShouldShowOperationGuideElement("buttonB_copy_copy2_copy", displayState),
                "split button in two player mode");
    ExpectFalse(ShouldShowOperationGuideElement("buttonTextB_copy2_copy2_copy", displayState),
                "split text in two player mode");
    ExpectTrue(ShouldShowOperationGuideElement("buttonA", displayState), "unrelated guide in two player mode");
}

void PauseShowsAllGuidesRegardlessOfStageProgress()
{
    for (int stage : {1, 2, 3}) {
        auto state = AtLocation(stage, 0);
        state.isPauseMenuOpen = true;
        state.isTwoPlayerMode = true;
        for (const char* id : {"buttonA", "buttonY", "weakAttackHoldGuide", "buttonB_copy_copy2"}) {
            ExpectTrue(ShouldShowOperationGuideElement(id, state), "pause opens every operation guide");
        }
    }
}

void ContextualRecoveryPreemptsAndExpiresWithCooldown()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.canRecover = {true, true};
    available.canSplit = true;
    guide.RequestSplit(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::Split, "split prompt started");
    guide.RequestRecovery(2);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::Recover && guide.GetPrompt()->playerNumber == 2,
               "recovery preempts and preserves player identity");
    guide.Update(0.2f, true, available);
    ExpectNear(1.0f, guide.GetPrompt()->opacity, 0.001f, "fade in reaches full opacity");
    guide.Update(2.6f, true, available);
    ExpectTrue(guide.GetPrompt()->opacity < 1.0f, "fade out is included in duration");
    guide.Update(0.21f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "prompt expires after total three seconds");
    guide.RequestRecovery(2);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "same player remains on cooldown");
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->playerNumber == 1, "other player has independent cooldown");
    guide.Dismiss(Kind::Recover, 1);
    ExpectFalse(guide.GetPrompt().has_value(), "recovery closes prompt immediately");
    guide.Update(15.0f, true, available);
    guide.RequestRecovery(2);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "ordinary damage does not repeat the stage introduction");
    guide.RequestRecovery(2, true);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "critical health can show a reminder after cooldown");
}

void SplitHintRefreshPreservesAnimationAndHasNoCooldown()
{
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.canSplit = true;
    guide.RequestSplit(1);
    guide.Update(0.0f, true, available);
    guide.Update(0.1f, true, available);
    const float opacity = guide.GetPrompt()->opacity;
    guide.RequestSplit(1);
    ExpectNear(opacity, guide.GetPrompt()->opacity, 0.001f, "repeated input preserves the current fade");
    guide.Update(0.1f, true, available);
    ExpectNear(1.0f, guide.GetPrompt()->opacity, 0.001f, "original fade continues without restarting");
    guide.Update(2.7f, true, available);
    const float fadingOpacity = guide.GetPrompt()->opacity;
    guide.RequestSplit(1);
    ExpectNear(fadingOpacity, guide.GetPrompt()->opacity, 0.001f, "refresh during fade out does not jump");
    guide.Update(0.2f, true, available);
    ExpectNear(1.0f, guide.GetPrompt()->opacity, 0.001f, "refreshed guide returns smoothly to full opacity");
    guide.Update(2.81f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "refreshed prompt expires after three seconds");
    guide.RequestSplit(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "another mistaken press can show the hint immediately");
    guide.Update(0.1f, false, available);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "pause or event clears the hint");
}

void RecoveryUsageSuppressesOnlyNormalHintsAndResetsPerStage()
{
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.canRecover = {true, true};
    guide.NotifyRecovered(1);
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "a player who used recovery needs no normal introduction");
    guide.RequestRecovery(1, true);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "critical health reminder remains available after using recovery");
    guide.NotifyRecovered(1);
    ExpectFalse(guide.GetPrompt().has_value(), "successful recovery dismisses the hint");
    guide.RequestRecovery(1, true);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "critical health respects the fifteen second cooldown");
    guide.RequestRecovery(2);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->playerNumber == 2,
               "other player's learning history is independent");
    guide.Reset();
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "new stage resets recovery usage and introduction history");
}

void UnavailableAndStaleHintsAreDiscarded()
{
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "no hint without HP and jewel eligibility");
    available.canRecover[0] = true;
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "unavailable request did not consume the stage introduction");
    guide.Reset();
    available.canSplit = true;
    guide.RequestSplit(1);
    guide.Update(8.1f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "stale queued hints are discarded");
}
} // namespace

void RegisterOperationGuideVisibilityTests(TestRegistry& tests)
{
    tests.emplace_back("OperationGuideVisibility.PauseShowsAllGuides", PauseShowsAllGuidesRegardlessOfStageProgress);
    tests.emplace_back("ContextualOperationGuide.RecoveryPriorityAndCooldown",
                       ContextualRecoveryPreemptsAndExpiresWithCooldown);
    tests.emplace_back("ContextualOperationGuide.SplitRefresh", SplitHintRefreshPreservesAnimationAndHasNoCooldown);
    tests.emplace_back("ContextualOperationGuide.RecoveryUsagePerStage",
                       RecoveryUsageSuppressesOnlyNormalHintsAndResetsPerStage);
    tests.emplace_back("ContextualOperationGuide.DiscardsInvalidHints", UnavailableAndStaleHintsAreDiscarded);
    tests.emplace_back("OperationGuideVisibility.StageOnePlanetZeroShowsBasicGuides",
                       StageOnePlanetZeroShowsOnlyBasicOperationGuides);
    tests.emplace_back("OperationGuideVisibility.StageOnePlanetThreeAddsDodge", StageOnePlanetThreeAddsDodgeGuide);
    tests.emplace_back("OperationGuideVisibility.StageOnePlanetOneAddsCombat", StageOnePlanetOneAddsCombatGuides);
    tests.emplace_back("OperationGuideVisibility.OtherLocationsShowAll", OtherLocationsShowAllOperationGuides);
    tests.emplace_back("OperationGuideVisibility.UGCShowsAll", UGCPlaytestsShowAllUnlockedActions);
    tests.emplace_back("OperationGuideVisibility.TwoPlayerModeHidesSoloConfigurationGuides",
                       TwoPlayerModeHidesSoloConfigurationGuides);
}
