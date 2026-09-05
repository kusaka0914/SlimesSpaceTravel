#pragma once

#include "actor/enemy/EnemyGrounding.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyStateMachine;
class EnemyStatus;
class Player;
class PhysicsSystem;
class MathUtils;

class EnemyMovement {
public:
    EnemyMovement(
        PhysicsSystem& physicsSystem,
        MathUtils& mathUtils);

    void UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime);
    void FaceNearestPlayerImmediately(Enemy& enemy, const EnemyStatus& status);

    void MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime);
    bool MoveTowardPlayerQuickly(
        Enemy& enemy,
        const EnemyStatus& status,
        float speed,
        float stopDistance,
        float deltaTime);
    float MoveAwayFromPlayerQuickly(
        Enemy& enemy,
        const EnemyStatus& status,
        float speed,
        float deltaTime);
    void MoveDuringAttacking(
        Enemy& enemy,
        const EnemyStatus& status,
        const EnemyStateMachine& stateMachine,
        float deltaTime);
    void MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime);
    void StartNormalHitKnockBack(
        Enemy& enemy,
        EnemyStatus& status);
    void MoveDuringDying(Enemy& enemy, float deltaTime);
    void ApplyAirDodgePush(
        Enemy& enemy,
        const glm::vec3& dodgeDirection,
        float pushSpeed,
        float pushDampingPerSecond);
    void ApplyAirComboLift(
        Enemy& enemy,
        float liftHeight);
    bool StartGravitySlam(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyStateMachine& stateMachine,
        Player& player,
        float downwardSpeed,
        float maximumDamage,
        float fullDamageHeight,
        float minimumDamageRatio,
        float groundImpactRadius,
        bool shouldPlayImpactFeedback);

    void LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);
    void UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);
    void SeparateAfterOverlappingEnemyLanding(
        Enemy& enemy,
        float deltaTime);
    void RestoreGroundedEnemyInsideMovementArea(Enemy& enemy);

    glm::vec3 CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta);

private:
    PhysicsSystem& mPhysicsSystem;
    MathUtils& mMathUtils;
    void ApplyGravityWithContinuousCollision(
        Enemy& enemy,
        EnemyStatus& status,
        float deltaTime);
    void UpdateAirDodgePushMovement(
        Enemy& enemy,
        float deltaTime);
    Enemy* FindGroundedEnemyBlockingFall(
        const Enemy& fallingEnemy,
        const glm::vec3& movementStart) const;
    bool TryPushGroundedEnemyAwayFromFall(
        Enemy& fallingEnemy,
        Enemy& groundedEnemy,
        float deltaTime);
    void ResolveGravitySlamImpact(
        Enemy& enemy,
        EnemyStatus& status);
    void ApplyGravitySlamGroundImpactDamage(
        Enemy& slammedEnemy,
        Player& attacker,
        float slamDamage) const;
    void StartGravitySlamImpactKnockBack(
        Enemy& enemy,
        EnemyStatus& status,
        const Player& attacker);

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
