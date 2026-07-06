#include "actor/player/PlayerCombat.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

bool PlayerCombat::IsAttacking() const
{
    return actionState == PlayerActionState::Attacking || actionState == PlayerActionState::StrongAttacking ||
           continuousAttackingTimer >= 0.0f || specialChargingTimer >= 0.0f || airAttackFloatingTimer >= 0.0f;
}

void PlayerCombat::StartAttacking(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerInput& input = context.input;

    actionState = PlayerActionState::Attacking;

    if (!player.GetOnGround() && input.wideAttackPressed) {
        attackKind = PlayerAttackKind::Wide;
        attackRange = wideAttackRange;
        attackAngle = wideAttackAngle;
        attackCooldownRemaining = attackCooldown;
        attack = wideAttack / 2.0f;
        airAttackFloatingTimer = 0.5f;
        isAirAttacking = true;

        Attack(context, deltaTime);
        return;
    }

    if (!player.GetOnGround()) {
        return;
    }

    if (input.attackPressed) {
        attackKind = PlayerAttackKind::Normal;
        attackRange = normalAttackRange;
        attackAngle = normalAttackAngle;
        attackCooldownRemaining = lastAttackCooldown;
        attack = normalAttack;
    } else if (input.wideAttackPressed) {
        attackKind = PlayerAttackKind::Wide;
        attackRange = wideAttackRange;
        attackAngle = wideAttackAngle;
        attackCooldownRemaining = attackCooldown;
        attack = wideAttack;
    }

    Attack(context, deltaTime);
}

void PlayerCombat::StartCharging(PlayerModuleContext& context, float deltaTime)
{
    actionState = PlayerActionState::Charging;
    attackPressTimer = defaultAttackPressTimer;

    context.player.GetGame()->GetAudioSystem()->PlaySE("air_charging_se");
}

void PlayerCombat::StartStrongAttacking(PlayerModuleContext& context, float deltaTime)
{
    actionState = PlayerActionState::StrongAttacking;
    attackKind = PlayerAttackKind::Strong;
    attackRange = strongAttackRange;
    attackAngle = normalAttackAngle;
    attackCooldownRemaining = lastAttackCooldown;
    attack = strongAttack;

    const float pressTime = std::min(1.0f, defaultAttackPressTimer - attackPressTimer / defaultAttackPressTimer);
    strongAttackTimer = defaultStrongAttackTimer * pressTime;

    isStrongAttacked = true;
}

void PlayerCombat::FinishCharging(PlayerModuleContext& context)
{
    context.player.GetGame()->OnPlayerFinishCharging(context.movement.playerNum);
    isCharged = true;
}

void PlayerCombat::FinishSpecialAttackCharging(PlayerModuleContext& context)
{
    specialChargingTimer = -1.0f;
    canSpecialAttack = false;
}

void PlayerCombat::Attack(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerMovement& movement = context.movement;

    std::vector<Enemy*> hitEnemies = FindHitEnemies(context);

    if (hitEnemies.empty()) {
        StartAfterAttackReaction(context);
        player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");

        if (attackComboIndex != 3) {
            return;
        }

        attackComboIndex = 0;
        return;
    }

    if (attackKind != PlayerAttackKind::Strong) {
        player.GetGame()->OnPlayerAttackHit(movement.playerNum);
        StartAfterAttackReaction(context);

        if (player.GetOnGround()) {
            for (Enemy* enemy : hitEnemies) {
                enemy->ApplyDamage(attack, &player);
            }
        } else {
            bool isHit = false;

            for (Enemy* enemy : hitEnemies) {
                if (enemy->GetOnGround()) {
                    continue;
                }

                enemy->ApplyDamage(attack, &player);
                isHit = true;
            }

            if (isHit) {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            } else {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");
            }

            return;
        }

        if (attackComboIndex != 3) {
            player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            return;
        }

        attackComboIndex = 0;
        player.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

        for (Enemy* enemy : hitEnemies) {
            if (enemy->GetOnGround()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        return;
    }

    player.GetGame()->GetAudioSystem()->PlaySE("attack_air_se");
    context.stateMachine.StartTired(context, 5.0f);

    for (Enemy* enemy : hitEnemies) {
        enemy->SetIsStrongAttacked(true);
        enemy->ApplyDamage(attack, &player);
        isStrongAttackHit = true;
    }
}

void PlayerCombat::WideAttack(PlayerModuleContext& context, float deltaTime)
{
    attackKind = PlayerAttackKind::Wide;
    attack = wideAttack;
    attackRange = wideAttackRange;
    attackAngle = wideAttackAngle;

    Attack(context, deltaTime);
}

void PlayerCombat::StrongAttack(PlayerModuleContext& context, float deltaTime)
{
    attackKind = PlayerAttackKind::Strong;
    attack = strongAttack;
    attackRange = strongAttackRange;
    attackAngle = normalAttackAngle;

    Attack(context, deltaTime);
}

void PlayerCombat::SpecialAttack(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;

    std::vector<Enemy*> enemies = FindHitEnemies(context);

    for (Enemy* enemy : enemies) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (enemy->GetOnGround()) {
            while (enemy->GetBreakCount()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        if (enemy->GetCanCountered()) {
            enemy->ApplyDamage(600, &player);
            enemy->FlipCanCountered();
            jewelCount = 2;
            player.GetGame()->GetAudioSystem()->PlaySE("just_attack_se");
        } else {
            enemy->ApplyDamage(300, &player);
        }
    }

    player.GetGame()->VibrateControllerForPlayer(context.movement.playerNum, 0, 40000, 1000);

    canSpecialAttack = false;
    attackCooldownRemaining = 1.0f;
}

void PlayerCombat::StartAfterAttackReaction(PlayerModuleContext& context)
{
    attackMoveLockRemaining = 0.2f;
    comboKeepTimer = attackMoveLockRemaining + 1.0f;

    if (context.player.GetOnGround()) {
        attackMotionTimer = defaultAttackMotionTimer;
    }

    attackComboIndex++;

    if (attackKind == PlayerAttackKind::Normal && attackComboIndex != 3) {
        attackComboIndex = 0;
        return;
    }

    if (attackKind == PlayerAttackKind::Strong) {
        context.stateMachine.StartTired(context, 5.0f);
        return;
    }

    if (attackComboIndex != 3) {
        return;
    }

    if (attackKind == PlayerAttackKind::Normal) {
        attackMoveLockRemaining = 1.0f;
    }

    if (attackKind == PlayerAttackKind::Wide && context.player.GetOnGround()) {
        attackCooldownRemaining = lastAttackCooldown;
        attackMoveLockRemaining = 0.8f;
    }
}

std::vector<Enemy*> PlayerCombat::FindHitEnemies(PlayerModuleContext& context)
{
    Player& player = context.player;

    std::vector<Enemy*> hitEnemies;

    if (!player.GetCurrentPlanet()) {
        return hitEnemies;
    }

    for (Enemy* enemy : player.GetCurrentPlanet()->GetEnemies()) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (attackKind == PlayerAttackKind::Strong && enemy->GetOnGround()) {
            continue;
        }

        const glm::vec3 enemyPos = enemy->GetPos();
        const glm::vec3 toEnemy =
            glm::normalize((enemyPos + enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f)) - player.GetPos());

        const float dist = glm::length(enemyPos - player.GetPos());
        const float dot = glm::dot(player.GetFacingForwardVec(), toEnemy);
        const float effectiveRange = attackRange + enemy->GetRadius();

        if (IsEnemyHitByAttack(context, dist, dot, effectiveRange)) {
            hitEnemies.push_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerCombat::IsEnemyHitByAttack(PlayerModuleContext& context, float dist, float dot, float effectiveRange)
{
    const float threshold = std::cos(attackAngle * 0.5f);
    return dist <= effectiveRange && dot >= threshold;
}

void PlayerCombat::StartSpecialAttackCharging(PlayerModuleContext& context)
{
    specialChargingTimer = 3.0f;
    attackRange = wideAttackRange;
    attackAngle = wideAttackAngle / 2.0f;
}

void PlayerCombat::StartContinuousAttacking(PlayerModuleContext& context)
{
    jewelCount--;
    continuousAttackingTimer = 6.0f;
}