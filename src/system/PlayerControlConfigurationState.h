#pragma once

#include <optional>

struct ControlledPlayerChange {
    int previousPlayerIndex = 0;
    int currentPlayerIndex = 0;
};

class PlayerControlConfigurationState {
public:
    bool IsPlayerSplit() const;
    int GetControlledPlayerIndex() const;

    std::optional<ControlledPlayerChange> SelectControlledPlayer(
        int playerIndex);
    void BeginPlayerSplit();
    std::optional<ControlledPlayerChange> EndPlayerSplit();
    void Reset();

    void ScheduleControlSwitchAfterBoarding(float delaySeconds);
    void CancelScheduledControlSwitch();
    std::optional<ControlledPlayerChange> UpdateScheduledControlSwitch(
        float deltaSeconds,
        bool isSecondPlayerJoined,
        int activePlayerIndex);

private:
    bool mIsPlayerSplit = false;
    int mControlledPlayerIndex = 0;
    float mPendingControlSwitchSeconds = -1.0f;
};
