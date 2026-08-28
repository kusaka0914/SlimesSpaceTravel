#include "system/PlayerControlConfigurationState.h"

#include <algorithm>

bool PlayerControlConfigurationState::IsPlayerSplit() const
{
    return mIsPlayerSplit;
}

int PlayerControlConfigurationState::GetControlledPlayerIndex() const
{
    return mControlledPlayerIndex;
}

std::optional<ControlledPlayerChange>
PlayerControlConfigurationState::SelectControlledPlayer(int playerIndex)
{
    if (playerIndex == mControlledPlayerIndex) {
        return std::nullopt;
    }

    const ControlledPlayerChange change{
        mControlledPlayerIndex,
        playerIndex,
    };
    mControlledPlayerIndex = playerIndex;
    return change;
}

void PlayerControlConfigurationState::BeginPlayerSplit()
{
    mIsPlayerSplit = true;
}

std::optional<ControlledPlayerChange>
PlayerControlConfigurationState::EndPlayerSplit()
{
    mIsPlayerSplit = false;
    mPendingControlSwitchSeconds = -1.0f;
    return SelectControlledPlayer(0);
}

void PlayerControlConfigurationState::Reset()
{
    mIsPlayerSplit = false;
    mControlledPlayerIndex = 0;
    mPendingControlSwitchSeconds = -1.0f;
}

void PlayerControlConfigurationState::ScheduleControlSwitchAfterBoarding(
    float delaySeconds)
{
    mPendingControlSwitchSeconds = std::max(0.0f, delaySeconds);
}

void PlayerControlConfigurationState::CancelScheduledControlSwitch()
{
    mPendingControlSwitchSeconds = -1.0f;
}

std::optional<ControlledPlayerChange>
PlayerControlConfigurationState::UpdateScheduledControlSwitch(
    float deltaSeconds,
    bool isSecondPlayerJoined,
    int activePlayerIndex)
{
    if (mPendingControlSwitchSeconds < 0.0f) {
        return std::nullopt;
    }

    mPendingControlSwitchSeconds -= deltaSeconds;
    if (mPendingControlSwitchSeconds > 0.0f) {
        return std::nullopt;
    }

    mPendingControlSwitchSeconds = -1.0f;
    if (isSecondPlayerJoined || !mIsPlayerSplit || activePlayerIndex < 0) {
        return std::nullopt;
    }

    return SelectControlledPlayer(activePlayerIndex);
}
