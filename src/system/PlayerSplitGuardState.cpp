#include "system/PlayerSplitGuardState.h"

#include <algorithm>

void PlayerSplitGuardState::Reset()
{
    mGuardCount = MaximumGuardCount;
    mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
}

void PlayerSplitGuardState::Update(float deltaTimeSeconds)
{
    if (mGuardCount >= MaximumGuardCount) {
        mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
        return;
    }

    mRecoveryRemainingSeconds -= std::max(0.0f, deltaTimeSeconds);
    constexpr float timerCompletionEpsilonSeconds = 0.000001f;
    while (mRecoveryRemainingSeconds <=
               timerCompletionEpsilonSeconds &&
           mGuardCount < MaximumGuardCount) {
        ++mGuardCount;
        mRecoveryRemainingSeconds += RecoveryIntervalSeconds;
    }

    if (mGuardCount >= MaximumGuardCount) {
        mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
    }
}

bool PlayerSplitGuardState::ConsumeOne()
{
    mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
    if (mGuardCount <= 0) {
        return false;
    }

    --mGuardCount;
    return true;
}
