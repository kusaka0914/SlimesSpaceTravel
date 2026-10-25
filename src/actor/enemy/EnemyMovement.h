#pragma once

#include "actor/enemy/EnemyGrounding.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyStateMachine;
class EnemyStatus;
class Player;
class PhysicsSystem;
class MathUtils;

// 追跡・外力・空中移動を衝突と惑星面の制限に合わせて反映し、叩き落としの着地ダメージも処理する。
// 位置・方向はワールド空間、距離はワールド単位、deltaTimeは秒。
class EnemyMovement {
public:
    EnemyMovement(PhysicsSystem& physicsSystem, MathUtils& mathUtils);

    void UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime);
    void FaceNearestPlayerImmediately(Enemy& enemy, const EnemyStatus& status);

    void MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime);

    // 接平面上で接近する。停止距離内への到達、移動不能、対象なしのいずれでもtrueを返す。
    bool MoveTowardPlayerQuickly(Enemy& enemy, const EnemyStatus& status, float speed, float stopDistance,
                                 float deltaTime);

    // 接平面上で対象から離れ、実際に移動したワールド距離を返す。移動できなければ0。
    float MoveAwayFromPlayerQuickly(Enemy& enemy, const EnemyStatus& status, float speed, float deltaTime);
    void MoveDuringAttacking(Enemy& enemy, const EnemyStatus& status, const EnemyStateMachine& stateMachine,
                             float deltaTime);
    void MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime);

    // 押し戻し方向を接平面へ補正し、上向き速度を設定して接地・通常着地判定を解除する。
    void StartNormalHitKnockBack(Enemy& enemy, EnemyStatus& status);
    void MoveDuringDying(Enemy& enemy, float deltaTime);

    // 接平面方向の押し出し速度を予約する。叩き落とし中や有効な方向を求められない場合は何もしない。
    void ApplyAirDodgePush(Enemy& enemy, const glm::vec3& dodgeDirection, float pushSpeed, float pushDampingPerSecond);

    // 頂点待機中は位置を持ち上げ、それ以外は上向き速度を加える。叩き落とし中は変更しない。
    void ApplyAirComboLift(Enemy& enemy, float liftHeight);

    // 下向き速度を設定してLaunchedへ遷移し、通常着地判定を停止する。既に実行中・上方向が無効ならfalse。
    // playerは着地時まで有効な非所有参照として保持し、開始高度に応じたダメージを着地時に適用する。
    bool StartGravitySlam(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player& player,
                          float downwardSpeed, float maximumDamage, float fullDamageHeight, float minimumDamageRatio,
                          float groundImpactRadius, bool shouldPlayImpactFeedback);

    // 記録済みの接地面から設定高度へ打ち上げ、Launchedへ遷移する。Gameへ通知し、ヒットストップも設定する。
    void LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);

    // 空中の外力・重力・頂点待機・着地を進め、着地時は叩き落としダメージと行動遷移も処理する。
    void UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);

    // 他の敵との重なりを許して落下した場合、着地後に接平面上へ移動して重なりを解消する。
    void SeparateAfterOverlappingEnemyLanding(Enemy& enemy, float deltaTime);

    // 楕円体の移動可能面を外れた生存・接地中の敵を、有効な最終接地位置へ戻して接線速度を除去する。
    void RestoreGroundedEnemyInsideMovementArea(Enemy& enemy);

    // 変位に対して衝突・惑星面・生存接地中の足場端を考慮した位置を返す。Enemyの位置はここでは更新しない。
    glm::vec3 CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta);

private:
    PhysicsSystem& mPhysicsSystem;
    MathUtils& mMathUtils;
    void ApplyGravityWithContinuousCollision(Enemy& enemy, EnemyStatus& status, float deltaTime);
    void UpdateAirDodgePushMovement(Enemy& enemy, float deltaTime);
    Enemy* FindGroundedEnemyBlockingFall(const Enemy& fallingEnemy, const glm::vec3& movementStart) const;
    bool TryPushGroundedEnemyAwayFromFall(Enemy& fallingEnemy, Enemy& groundedEnemy, float deltaTime);
    void ResolveGravitySlamImpact(Enemy& enemy, EnemyStatus& status);
    void ApplyGravitySlamGroundImpactDamage(Enemy& slammedEnemy, Player& attacker, float slamDamage) const;
    void StartGravitySlamImpactKnockBack(Enemy& enemy, EnemyStatus& status, const Player& attacker);

    EnemyGrounding mGrounding;
    glm::vec3 mAirDodgePushVelocity{0.0f};
    float mAirDodgePushDampingPerSecond = 8.0f;
    float mEnemyBlockedFallSeconds = 0.0f;
    bool mShouldSeparateAfterLanding = false;
    bool mHasCompletedLaunchApexWait = false;
    bool mIsGravitySlamActive = false;
    bool mShouldPlayGravitySlamImpactFeedback = false;
    Player* mGravitySlamAttacker = nullptr;
    float mGravitySlamStartHeight = 0.0f;
    float mGravitySlamMaximumDamage = 0.0f;
    float mGravitySlamFullDamageHeight = 1.0f;
    float mGravitySlamMinimumDamageRatio = 0.0f;
    float mGravitySlamGroundImpactRadius = 0.0f;
};
