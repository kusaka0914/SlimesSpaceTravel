#pragma once

// ボタンの短押し・長押しをPlayerごとに判定する。時間はゲーム内の秒数で、ポーズ中は進めない。
class PlayerCombatInput {
public:
    static constexpr float longPressSeconds = 0.7f;

    struct Requests {
        bool weakAttack = false;
        bool strongAttack = false;
        bool continuousAttack = false;
        bool chargeAttack = false;
        bool releaseChargedAttack = false;
        bool recover = false;
    };

    // 各要求はその更新で一度だけ返す。Yは押下時に弱攻撃、長押し成立時に連続攻撃を要求する。
    // Xは短押し・溜め中とも解放を通知する。溜め中の解放は短押し操作へ流さない。
    Requests Update(bool weakPressed, bool strongPressed, bool recoverPressed, bool isAssistStyle,
                    bool isPreparingChargedAttack, float deltaTime);

    // 被弾・操作対象変更などで中断し、各ボタンを離すまで新しい要求を出さない。
    void SuppressUntilReleased();

private:
    struct ButtonState {
        bool wasPressed = false;
        bool isSuppressed = false;
        bool longPressHandled = false;
        float heldSeconds = 0.0f;
    };

    ButtonState mWeak;
    ButtonState mStrong;
    bool mRecoverWasPressed = false;
};
