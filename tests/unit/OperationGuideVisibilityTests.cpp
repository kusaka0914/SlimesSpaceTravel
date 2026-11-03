#include "support/TestSupport.h"

#include "gfx/ui/ContextualOperationGuide.h"
#include "gfx/ui/OperationGuideVisibility.h"

#include <algorithm>

namespace {
OperationGuideDisplayState AtLocation(int stageNumber, int planetNumber)
{
    OperationGuideDisplayState displayState;
    displayState.currentStageNumber = stageNumber;
    displayState.currentPlanetNumber = planetNumber;
    return displayState;
}

void NormalPlayShowsOnlyMenuOperationGuide()
{
    for (int stage : {0, 1, 2, 3}) {
        for (int planet : {0, 1, 2, 3}) {
            for (bool twoPlayers : {false, true}) {
                auto state = AtLocation(stage, planet);
                state.isTwoPlayerMode = twoPlayers;
                for (const char* id : {"buttonA", "buttonB", "buttonX", "buttonY", "weakAttackHoldGuide",
                                       "strongAttackHoldGuide", "buttonB_copy_copy2", "buttonB_copy_copy2_copy"}) {
                    ExpectFalse(ShouldShowOperationGuideElement(id, state), "normal play hides permanent controls");
                }
                ExpectTrue(ShouldShowOperationGuideElement("buttonA_copy", state), "menu icon remains visible");
                ExpectTrue(ShouldShowOperationGuideElement("buttonTextB_copy", state), "menu label remains visible");
            }
        }
    }
}

void UGCPlaytestsShowAllUnlockedActions()
{
    for (int stage : {0, 1, 2}) {
        OperationGuideDisplayState ugcPlaytest = AtLocation(stage, 0);
        ugcPlaytest.isUGCPlaytestActive = true;
        ExpectTrue(ShouldShowOperationGuideElement("buttonY", ugcPlaytest), "combat guide in UGC playtest");
    }
}

void TwoPlayerModeHidesSoloConfigurationGuides()
{
    OperationGuideDisplayState displayState = AtLocation(1, 1);
    displayState.isUGCPlaytestActive = true;
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
    for (int stage : {0, 1, 2, 3}) {
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
               "recovery is first in priority order and preserves player identity");
    ExpectEqual<std::size_t>(2, guide.GetPrompts().size(), "independent hints remain visible together");
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
void BlockedSplitHasPriorityAndRepeatedPressDoesNotExtendIt()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.canRecover[0] = true;
    available.canExplainBlockedSplit = true;
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    guide.RequestSplitBlocked(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::SplitBlocked,
               "failed action feedback preempts recovery");
    guide.Update(0.2f, true, available);
    ExpectNear(1.0f, guide.GetPrompt()->opacity, 0.001f, "feedback fades in");
    guide.Update(2.6f, true, available);
    const float opacity = guide.GetPrompt()->opacity;
    guide.RequestSplitBlocked(1);
    ExpectNear(opacity, guide.GetPrompt()->opacity, 0.001f, "repeat does not reset fade");
    guide.Update(0.21f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "repeat did not extend three second lifetime");
    guide.RequestSplitBlocked(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "another failed attempt can show feedback again");
    available.canExplainBlockedSplit = false;
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "successful split or player change removes stale feedback");
}

void InsufficientJewelsTracksRequiredCountWithoutRefreshing()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.activePlayers = {true, true};
    available.canRecover[0] = true;
    guide.RequestRecovery(1);
    guide.Update(0.0f, true, available);
    guide.RequestInsufficientJewels(2, 1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::InsufficientJewels &&
                   guide.GetPrompt()->playerNumber == 2,
               "resource failure preempts optional hint for correct player");
    guide.Update(0.2f, true, available);
    guide.RequestInsufficientJewels(2, 2);
    ExpectNear(1.0f, guide.GetPrompt()->opacity, 0.001f, "repeat does not restart fade");
    available.jewelCounts[1] = 1;
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "one jewel is still insufficient for a two jewel skill");
    available.jewelCounts[1] = 2;
    guide.Update(0.0f, true, available);
    const auto remainingPrompts = guide.GetPrompts();
    ExpectTrue(remainingPrompts.size() == 1 && remainingPrompts.front().kind == Kind::Recover,
               "enough jewels clears only the obsolete failure message");
    available.jewelCounts[1] = 0;
    guide.RequestInsufficientJewels(2, 1);
    guide.Update(0.0f, true, available);
    guide.Update(2.8f, true, available);
    guide.RequestInsufficientJewels(2, 1);
    guide.Update(0.21f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "repeat does not extend the three second lifetime");
}

void BlockedCameraFeedbackExpiresAndFollowsPlayerZone()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.activePlayers = {true, true};
    available.canExplainBlockedCamera[1] = true;
    guide.RequestCameraControlBlocked(1);
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "another player's zone cannot enable a camera hint");
    guide.RequestCameraControlBlocked(2);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::CameraControlBlocked &&
                   guide.GetPrompt()->playerNumber == 2,
               "blocked camera input identifies the player");
    guide.Update(2.8f, true, available);
    guide.RequestCameraControlBlocked(2);
    guide.Update(0.21f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "repeated camera input does not extend the hint");
    guide.RequestCameraControlBlocked(2);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "a later blocked camera attempt can show the hint again");
    available.canExplainBlockedCamera[1] = false;
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "leaving the camera zone clears the hint");
}

void MergeDistanceFeedbackStopsWhenAvailableAndCanRepeat()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.canExplainMergeTooFar = true;
    guide.RequestMergeTooFar(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt() && guide.GetPrompt()->kind == Kind::MergeTooFar, "distance failure is displayed");
    guide.Update(2.8f, true, available);
    guide.RequestMergeTooFar(1);
    guide.Update(0.21f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "held or repeated merge does not prolong feedback");
    guide.RequestMergeTooFar(1);
    guide.Update(0.0f, true, available);
    ExpectTrue(guide.GetPrompt().has_value(), "later merge failure has no cooldown");
    available.canExplainMergeTooFar = false;
    guide.Update(0.0f, true, available);
    ExpectFalse(guide.GetPrompt().has_value(), "approaching the other slime clears the hint");
}

void ThreePromptsKeepIndependentTimersAndPlayerCapacity()
{
    using Kind = ContextualOperationGuide::Kind;
    ContextualOperationGuide guide;
    ContextualOperationGuide::Availability available;
    available.activePlayers = {true, true};
    available.canRecover = {true, true};
    available.canExplainBlockedCamera = {true, true};
    available.canSplit = true;
    available.canExplainBlockedSplit = true;
    available.canExplainMergeTooFar = true;

    guide.RequestSplit(1);
    guide.Update(0.0f, true, available);
    guide.Update(1.0f, true, available);
    guide.RequestRecovery(1);
    guide.RequestCameraControlBlocked(1);
    guide.RequestMergeTooFar(2);
    guide.Update(0.0f, true, available);
    ExpectEqual<std::size_t>(4, guide.GetPrompts().size(), "each player has independent display slots");
    guide.Update(2.01f, true, available);
    const auto afterExpiry = guide.GetPrompts();
    ExpectTrue(afterExpiry.size() == 3 && std::none_of(afterExpiry.begin(), afterExpiry.end(),
                                                       [](const auto& prompt) { return prompt.kind == Kind::Split; }),
               "one prompt expiring does not restart or dismiss the other prompts");

    guide.Reset();
    guide.RequestSplit(1);
    guide.RequestRecovery(1);
    guide.RequestCameraControlBlocked(1);
    guide.Update(0.0f, true, available);
    guide.RequestSplitBlocked(1);
    guide.Update(0.0f, true, available);
    const auto prioritized = guide.GetPrompts();
    ExpectTrue(prioritized.size() == 3 && prioritized.front().kind == Kind::SplitBlocked &&
                   std::none_of(prioritized.begin(), prioritized.end(),
                                [](const auto& prompt) { return prompt.kind == Kind::Split; }),
               "at capacity, action failures replace only the lowest-priority hint");
    guide.RequestSplitBlocked(1);
    guide.RequestMergeTooFar(2);
    guide.RequestCameraControlBlocked(2);
    guide.RequestRecovery(2);
    guide.Update(0.0f, true, available);
    ExpectEqual<std::size_t>(6, guide.GetPrompts().size(),
                             "repeated requests do not duplicate, and player two has three slots");
    guide.Update(0.0f, false, available);
    ExpectTrue(guide.GetPrompts().empty(), "pause clears every displayed and pending prompt");
}

} // namespace

void RegisterOperationGuideVisibilityTests(TestRegistry& tests)
{
    tests.emplace_back("ContextualOperationGuide.ThreePrompts", ThreePromptsKeepIndependentTimersAndPlayerCapacity);
    tests.emplace_back("ContextualOperationGuide.BlockedCamera", BlockedCameraFeedbackExpiresAndFollowsPlayerZone);
    tests.emplace_back("ContextualOperationGuide.InsufficientJewels",
                       InsufficientJewelsTracksRequiredCountWithoutRefreshing);
    tests.emplace_back("ContextualOperationGuide.MergeDistance", MergeDistanceFeedbackStopsWhenAvailableAndCanRepeat);
    tests.emplace_back("ContextualOperationGuide.BlockedSplitPriorityAndNoRefresh",
                       BlockedSplitHasPriorityAndRepeatedPressDoesNotExtendIt);
    tests.emplace_back("OperationGuideVisibility.PauseShowsAllGuides", PauseShowsAllGuidesRegardlessOfStageProgress);
    tests.emplace_back("ContextualOperationGuide.RecoveryPriorityAndCooldown",
                       ContextualRecoveryPreemptsAndExpiresWithCooldown);
    tests.emplace_back("ContextualOperationGuide.SplitRefresh", SplitHintRefreshPreservesAnimationAndHasNoCooldown);
    tests.emplace_back("ContextualOperationGuide.RecoveryUsagePerStage",
                       RecoveryUsageSuppressesOnlyNormalHintsAndResetsPerStage);
    tests.emplace_back("ContextualOperationGuide.DiscardsInvalidHints", UnavailableAndStaleHintsAreDiscarded);
    tests.emplace_back("OperationGuideVisibility.NormalPlayShowsOnlyMenu", NormalPlayShowsOnlyMenuOperationGuide);
    tests.emplace_back("OperationGuideVisibility.UGCShowsAll", UGCPlaytestsShowAllUnlockedActions);
    tests.emplace_back("OperationGuideVisibility.TwoPlayerModeHidesSoloConfigurationGuides",
                       TwoPlayerModeHidesSoloConfigurationGuides);
}
