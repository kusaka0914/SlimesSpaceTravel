#include "actor/player/PlayerCombatInput.h"

#include <algorithm>

PlayerCombatInput::Requests PlayerCombatInput::Update(bool weakPressed, bool strongPressed, bool recoverPressed,
                                                      bool isAssistStyle, bool isPreparingChargedAttack,
                                                      float deltaTime)
{
    Requests requests;
    const float elapsedSeconds = std::max(0.0f, deltaTime);
    const auto updateButton = [elapsedSeconds](ButtonState& button, bool pressed) {
        if (button.isSuppressed) {
            if (!pressed) {
                button = {};
            }
            return false;
        }
        if (!pressed) {
            return true;
        }
        if (!button.wasPressed) {
            button.heldSeconds = 0.0f;
            button.longPressHandled = false;
        }
        button.heldSeconds = std::min(longPressSeconds, button.heldSeconds + elapsedSeconds);
        return true;
    };

    if (updateButton(mWeak, weakPressed)) {
        requests.weakAttack = weakPressed && !mWeak.wasPressed && !isPreparingChargedAttack;
        if (weakPressed && !mWeak.longPressHandled && mWeak.heldSeconds >= longPressSeconds) {
            mWeak.longPressHandled = true;
            requests.continuousAttack = !isPreparingChargedAttack;
        }
        if (!weakPressed) {
            mWeak = {};
        }
    }
    mWeak.wasPressed = weakPressed;

    if (updateButton(mStrong, strongPressed)) {
        if (isPreparingChargedAttack) {
            requests.releaseChargedAttack = !strongPressed && mStrong.wasPressed;
            // 発射・溜め中の押下を、後の解放時に強攻撃や回復として再利用しない。
            mStrong.longPressHandled = true;
        } else if (strongPressed && !mStrong.longPressHandled && mStrong.heldSeconds >= longPressSeconds) {
            mStrong.longPressHandled = true;
            requests.chargeAttack = true;
        } else if (!strongPressed && mStrong.wasPressed && !mStrong.longPressHandled) {
            requests.strongAttack = !isAssistStyle;
            requests.recover = isAssistStyle;
        }
        if (!strongPressed) {
            mStrong = {};
        }
    }
    mStrong.wasPressed = strongPressed;

    requests.recover |= recoverPressed && !mRecoverWasPressed && !isPreparingChargedAttack;
    mRecoverWasPressed = recoverPressed;
    return requests;
}

void PlayerCombatInput::SuppressUntilReleased()
{
    mWeak = {.isSuppressed = true};
    mStrong = {.isSuppressed = true};
    mRecoverWasPressed = true;
}
