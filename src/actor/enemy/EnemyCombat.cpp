#include "actor/enemy/EnemyCombat.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyAttackGeometry.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"

#include <cmath>
#include <glm/glm.hpp>

void EnemyCombat::ApplyBreak(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyStateMachine& stateMachine,
                             float deltaTime, bool isAllBreak)
{
    if (!stateMachine.IsAlive(enemy)) {
        return;
    }

    if (isAllBreak) {
        status.BreakAll();
    } else {
        status.DecrementBreakCount();
    }

    enemy.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

    // 空中の敵を再度打ち上げると、ガード破壊のたびに浮き直してしまう。
    // ガード数は減らすが、打ち上げ開始は地上にいる敵だけに限定する。
    if (status.IsBreakCountEmpty() && enemy.IsOnGround()) {
        movement.LaunchIntoAir(enemy, status, stateMachine, deltaTime);
        return;
    }
}

void EnemyCombat::TryApplyAttack(
    Enemy& enemy,
    EnemyStatus& status,
    const EnemyStateMachine& stateMachine,
    const glm::vec3& movementStart,
    float deltaTime)
{
    if (!stateMachine.IsProgressing(status)) {
        return;
    }

    const EnemyAttackFrame& attackFrame =
        stateMachine.GetActiveAttackFrame();
    PhysicsSystem* physicsSystem =
        enemy.GetGame()->GetPhysicsSystem();
    if (!physicsSystem) {
        return;
    }

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player) {
            continue;
        }

        if (!player->IsAlive()) {
            continue;
        }

        if (status.HasHitPlayer(player)) {
            continue;
        }

        if (!CanHitPlayer(enemy, player)) {
            continue;
        }

        const glm::vec3 movementStartToPlayer =
            player->GetPos() - movementStart;
        const glm::vec3 planarPlayerOffset =
            movementStartToPlayer -
            attackFrame.up *
                glm::dot(
                    movementStartToPlayer,
                    attackFrame.up);
        constexpr float minimumForwardDistance = 0.0001f;
        if (glm::dot(
                planarPlayerOffset,
                attackFrame.forward) <=
            minimumForwardDistance) {
            continue;
        }

        if (!physicsSystem->
                DoesActorModelSweepOverlapActorCollision(
                    enemy,
                    movementStart,
                    *player)) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
    }
}

void EnemyCombat::TryApplyFanAttack(
    Enemy& enemy,
    EnemyStatus& status,
    const EnemyStateMachine& stateMachine,
    float range,
    float angleRadians,
    float deltaTime)
{
    const EnemyAttackFrame& attackFrame =
        stateMachine.GetActiveAttackFrame();

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player || !player->IsAlive() || status.HasHitPlayer(player)) {
            continue;
        }

        if (!CanHitPlayer(enemy, player)) {
            continue;
        }

        if (!IsPositionInsideFanAttack(
                attackFrame,
                player->GetPos(),
                range,
                angleRadians)) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
    }
}

void EnemyCombat::TryApplyGroundRadialAttack(
    Enemy& enemy,
    EnemyStatus& status,
    const EnemyStateMachine& stateMachine,
    float range,
    float deltaTime)
{
    const EnemyAttackFrame& attackFrame =
        stateMachine.GetActiveAttackFrame();

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player || !player->IsAlive() || !player->GetOnGround() ||
            status.HasHitPlayer(player)) {
            continue;
        }

        if (!CanHitPlayer(enemy, player)) {
            continue;
        }

        if (!IsPositionInsideRadialAttack(
                attackFrame,
                player->GetPos(),
                range)) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
    }
}

bool EnemyCombat::IsPlayerInRange(const Enemy& enemy, Player* player, float range) const
{
    if (!CanHitPlayer(enemy, player)) {
        return false;
    }

    const float distToPlayer = glm::length(player->GetPos() - enemy.GetPos());
    return distToPlayer <= range;
}

bool EnemyCombat::CanHitPlayer(
    const Enemy& enemy,
    const Player* player) const
{
    const Planet* planet = enemy.GetCurrentPlanet();
    if (!player ||
        !player->GetIsActive() ||
        !planet ||
        player->GetCurrentPlanet() != planet ||
        !planet->ArePositionsOnSameSurfaceFace(
            enemy.GetPos(),
            player->GetPos())) {
        return false;
    }
    return true;
}
