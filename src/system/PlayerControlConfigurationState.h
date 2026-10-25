#pragma once

#include <optional>

struct ControlledPlayerChange {
    int previousPlayerIndex = 0;
    int currentPlayerIndex = 0;
};

// 分裂状態と操作対象のインデックス、乗船後の遅延切替予約を保持する。
class PlayerControlConfigurationState {
public:
    bool IsPlayerSplit() const;
    int GetControlledPlayerIndex() const;

    // 0始まりの操作対象を設定し、変更前後を返す。同じ対象ならnullopt。インデックスの検証は呼び出し側で行う。
    std::optional<ControlledPlayerChange> SelectControlledPlayer(int playerIndex);
    void BeginPlayerSplit();

    // 分裂と遅延切替予約を解除し、操作対象を先頭へ戻す。対象が変わらなければnullopt。
    std::optional<ControlledPlayerChange> EndPlayerSplit();
    void Reset();

    void ScheduleControlSwitchAfterBoarding(float delaySeconds);
    void CancelScheduledControlSwitch();

    // 秒単位で予約を進める。期限到達時に単独分裂中ならactivePlayerIndexへ切り替え、変更前後を返す。
    std::optional<ControlledPlayerChange> UpdateScheduledControlSwitch(float deltaSeconds, bool isSecondPlayerJoined,
                                                                       int activePlayerIndex);

private:
    bool mIsPlayerSplit = false;
    int mControlledPlayerIndex = 0;
    float mPendingControlSwitchSeconds = -1.0f;
};
