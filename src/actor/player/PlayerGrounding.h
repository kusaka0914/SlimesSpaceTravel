#pragma once

class Player;
class PlayerCombat;
class PlayerMovement;
class PhysicsSystem;

// 着地時の行動回復、接地レイ失敗時の補正、補助的な接地探索を担当する。
class PlayerGrounding {
public:
    explicit PlayerGrounding(PhysicsSystem& physicsSystem);

    // 空中攻撃と回避の使用状態をリセットし、Game::OnLandedを通知する。
    void OnLanded(Player& player, PlayerMovement& movement, PlayerCombat& combat);

    // レイの猶予時間切れ時、球・楕円体の惑星では上方向を補正して速度を0にし、猶予を再設定する。
    void OnUpVecUpdateFailed(Player& player);
    void OnCastSucceeded();

    // upOffset上方からdownLength下方へワールド距離で探索し、成功時は位置・接地状態を更新して速度を0にする。
    // 重力方向に影響しない面では、接地せず上方向と速度だけを補正する。通常の着地通知は行わない。
    void SnapToGround(Player& player, float upOffset, float downLength);

    void UpdateRayCastTimer(float deltaTime);
    void ResetRayCastTimer() { mRayCastTimer = 0.5f; }
    void SetRayCastTimer(float rayCastTimer) { mRayCastTimer = rayCastTimer; }
    float GetRayCastTimer() const { return mRayCastTimer; }

private:
    PhysicsSystem& mPhysicsSystem;
    float mRayCastTimer = 0.5f;
};
