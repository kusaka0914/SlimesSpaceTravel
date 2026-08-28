#include "TestSupport.h"

#include "system/PlayerControlConfigurationState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void SplitAndMergeTrackTheSelectedPlayer()
{
    PlayerControlConfigurationState state;

    state.BeginPlayerSplit();
    const std::optional<ControlledPlayerChange> splitSelection =
        state.SelectControlledPlayer(1);
    const std::optional<ControlledPlayerChange> mergeSelection =
        state.EndPlayerSplit();

    ExpectFalse(state.IsPlayerSplit(), "split state after merge");
    ExpectTrue(splitSelection.has_value(), "split selection change");
    ExpectEqual(0, splitSelection->previousPlayerIndex, "split previous player");
    ExpectEqual(1, splitSelection->currentPlayerIndex, "split current player");
    ExpectTrue(mergeSelection.has_value(), "merge selection change");
    ExpectEqual(1, mergeSelection->previousPlayerIndex, "merge previous player");
    ExpectEqual(0, mergeSelection->currentPlayerIndex, "merge current player");
}

void SelectingCurrentPlayerDoesNotCreateChange()
{
    PlayerControlConfigurationState state;

    const std::optional<ControlledPlayerChange> change =
        state.SelectControlledPlayer(0);

    ExpectFalse(change.has_value(), "unchanged selection");
}

void ScheduledSwitchWaitsForDelayAndUsesActivePlayer()
{
    PlayerControlConfigurationState state;
    state.BeginPlayerSplit();
    state.SelectControlledPlayer(1);
    state.ScheduleControlSwitchAfterBoarding(0.5f);

    ExpectFalse(
        state.UpdateScheduledControlSwitch(0.25f, false, 0).has_value(),
        "switch before delay");

    const std::optional<ControlledPlayerChange> change =
        state.UpdateScheduledControlSwitch(0.25f, false, 0);

    ExpectTrue(change.has_value(), "switch after delay");
    ExpectEqual(0, state.GetControlledPlayerIndex(), "active player selection");
}

void ScheduledSwitchIsDiscardedWhenSplitEnds()
{
    PlayerControlConfigurationState state;
    state.BeginPlayerSplit();
    state.ScheduleControlSwitchAfterBoarding(0.0f);
    state.EndPlayerSplit();

    ExpectFalse(
        state.UpdateScheduledControlSwitch(0.0f, false, 1).has_value(),
        "discarded switch");
}

}

void RegisterPlayerControlConfigurationStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlayerControlConfigurationState.SplitAndMergeTrackTheSelectedPlayer",
        SplitAndMergeTrackTheSelectedPlayer);
    tests.emplace_back(
        "PlayerControlConfigurationState.SelectingCurrentPlayerDoesNotCreateChange",
        SelectingCurrentPlayerDoesNotCreateChange);
    tests.emplace_back(
        "PlayerControlConfigurationState.ScheduledSwitchWaitsForDelayAndUsesActivePlayer",
        ScheduledSwitchWaitsForDelayAndUsesActivePlayer);
    tests.emplace_back(
        "PlayerControlConfigurationState.ScheduledSwitchIsDiscardedWhenSplitEnds",
        ScheduledSwitchIsDiscardedWhenSplitEnds);
}
