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

#include <array>
#include <cmath>
#include <glm/glm.hpp>

namespace {
std::array<glm::vec3, 9> GetPlayerSurfaceCollisionSamples(
    const Player& player,
    const PhysicsSystem& physicsSystem)
{
    const glm::vec3 up = glm::normalize(player.GetUpVec());
    glm::vec3 forward = player.GetFacingForwardVec();
    forward -= up * glm::dot(forward, up);
    if (glm::dot(forward, forward) < 0.000001f) {
        forward = player.GetForwardVec();
        forward -= up * glm::dot(forward, up);
    }
    forward = glm::normalize(forward);
    const glm::vec3 left = glm::normalize(glm::cross(up, forward));
    const float scale = player.GetCollisionScaleMultiplier();
    const float halfForward =
        physicsSystem.GetPlayerCollisionDepth() * scale * 0.5f;
    const float halfSide =
        physicsSystem.GetPlayerCollisionWidth() * scale * 0.5f;
    const glm::vec3 position = player.GetPos();

    return {
        position,
        position + forward * halfForward,
        position - forward * halfForward,
        position + left * halfSide,
        position - left * halfSide,
        position + forward * halfForward + left * halfSide,
        position + forward * halfForward - left * halfSide,
        position - forward * halfForward + left * halfSide,
        position - forward * halfForward - left * halfSide};
}
}

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
    float deltaTime)
{
    if (!stateMachine.IsProgressing(status)) {
        return;
    }

    const EnemyAttackFrame& attackFrame =
        stateMachine.GetActiveAttackFrame();
    const EnemyMeleeAttackPreviewArea attackArea =
        CalculateEnemyMeleeAttackPreviewArea(enemy, attackFrame);
    EnemyAttackFrame meleeAttackFrame = attackFrame;
    meleeAttackFrame.origin +=
        meleeAttackFrame.forward * attackArea.forwardStartOffset;
    const Planet* planet = enemy.GetCurrentPlanet();
    PhysicsSystem* physicsSystem = enemy.GetGame()->GetPhysicsSystem();

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

        bool isInsideAttack = false;
        if (planet && planet->GetPlanetShape() ==
                          Planet::PlanetShape::Sphere &&
            physicsSystem) {
            for (const glm::vec3& samplePosition :
                 GetPlayerSurfaceCollisionSamples(*player, *physicsSystem)) {
                if (IsPositionInsideSphereSurfaceMeleeAttack(
                        *planet,
                        meleeAttackFrame,
                        samplePosition,
                        attackArea.forwardLength,
                        attackArea.halfWidth)) {
                    isInsideAttack = true;
                    break;
                }
            }
        } else {
            isInsideAttack = IsPositionInsideMeleeAttack(
                meleeAttackFrame,
                player->GetPos(),
                attackArea.forwardLength,
                attackArea.halfWidth);
        }
        if (!isInsideAttack) {
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
    EnemyAttackFrame fanAttackFrame = attackFrame;
    fanAttackFrame.origin +=
        fanAttackFrame.forward *
        CalculateEnemyAttackFrontOffset(enemy, attackFrame);

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player || !player->IsAlive() || status.HasHitPlayer(player)) {
            continue;
        }

        if (!CanHitPlayer(enemy, player)) {
            continue;
        }

        const Planet* planet = enemy.GetCurrentPlanet();
        bool isInsideAttack = false;
        if (planet && planet->GetPlanetShape() ==
                          Planet::PlanetShape::Sphere) {
            PhysicsSystem* physicsSystem =
                enemy.GetGame()->GetPhysicsSystem();
            if (physicsSystem) {
                for (const glm::vec3& samplePosition :
                     GetPlayerSurfaceCollisionSamples(
                         *player, *physicsSystem)) {
                    if (IsPositionInsideSphereSurfaceFanAttack(
                            *planet,
                            fanAttackFrame,
                            samplePosition,
                            range,
                            angleRadians)) {
                        isInsideAttack = true;
                        break;
                    }
                }
            }
        } else {
            isInsideAttack = IsPositionInsideFanAttack(
                fanAttackFrame,
                player->GetPos(),
                range,
                angleRadians);
        }
        if (!isInsideAttack) {
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

        const Planet* planet = enemy.GetCurrentPlanet();
        bool isInsideAttack = false;
        if (planet && planet->GetPlanetShape() ==
                          Planet::PlanetShape::Sphere) {
            PhysicsSystem* physicsSystem =
                enemy.GetGame()->GetPhysicsSystem();
            if (physicsSystem) {
                for (const glm::vec3& samplePosition :
                     GetPlayerSurfaceCollisionSamples(
                         *player, *physicsSystem)) {
                    if (IsPositionInsideSphereSurfaceRadialAttack(
                            *planet,
                            attackFrame,
                            samplePosition,
                            range)) {
                        isInsideAttack = true;
                        break;
                    }
                }
            }
        } else {
            isInsideAttack = IsPositionInsideRadialAttack(
                attackFrame,
                player->GetPos(),
                range);
        }
        if (!isInsideAttack) {
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
