#pragma once

#include "actor/enemy/EnemyGrounding.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyStateMachine;
class EnemyStatus;
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
    void MoveDuringDying(Enemy& enemy, float deltaTime);
    void ApplyAirDodgePush(
        Enemy& enemy,
        const glm::vec3& dodgeDirection,
        float pushSpeed,
        float pushDampingPerSecond);

    void LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);
    void UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);

    glm::vec3 CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta);

private:
    PhysicsSystem& mPhysicsSystem;
    MathUtils& mMathUtils;
    void ApplyGravityWithContinuousCollision(
        Enemy& enemy,
        float deltaTime);
    void UpdateAirDodgePushMovement(
        Enemy& enemy,
        float deltaTime);

    EnemyGrounding mGrounding;
    glm::vec3 mAirDodgePushVelocity{0.0f};
    float mAirDodgePushDampingPerSecond = 8.0f;
};
