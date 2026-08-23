#include "actor/player/PlayerDamageHandler.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"
#include "system/AudioSystem.h"

#include <random>

namespace {
constexpr double damageTiredChance = 0.10;
constexpr float chargedAttackFailureLockDurationSeconds = 10.0f;
constexpr float airborneDamageMultiplier = 2.0f;

bool ShouldStartRandomDamageTiredLock()
{
    thread_local std::mt19937 randomEngine(std::random_device{}());
    std::bernoulli_distribution tiredRoll(damageTiredChance);
    return tiredRoll(randomEngine);
}

void ApplyDamageAndKnockBack(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerStateMachine& stateMachine,
    PlayerCombat& combat,
    PlayerStatus& status,
    const glm::vec3& damageSourcePosition,
    float damage)
{
    if (status.IsInvincible()) {
        return;
    }

    const bool wasChargedAttackInterrupted =
        combat.GetCanSpecialAttack();
    const bool shouldStartTiredLock =
        wasChargedAttackInterrupted ||
        ShouldStartRandomDamageTiredLock();
    if (shouldStartTiredLock) {
        combat.StartTiredLock(
            status,
            movement,
            chargedAttackFailureLockDurationSeconds);
    }

    const float appliedDamage =
        player.GetOnGround()
            ? damage
            : damage * airborneDamageMultiplier;
    status.TakeDamage(appliedDamage);
    movement.StartKnockBack(damageSourcePosition);
    movement.ClearStrongAttackDirectionOverride();
    stateMachine.ClearAttackDirectionTarget();
    player.SetShouldJudgeLanding(true);
    stateMachine.ChangeState(PlayerActionState::KnockedBack);

    player.GetGame()->OnPlayerApplyDamage(movement.GetPlayerNum());

    combat.CancelSpecialAttack();
    input.ClearAttackBuffer();
    input.SyncAttackButtonPrev();
}
} // namespace

void PlayerDamageHandler::Apply(Player& player, PlayerInput& input, PlayerMovement& movement,
                                PlayerStateMachine& stateMachine, PlayerCombat& combat,
                                PlayerJewelGauge& jewelGauge, PlayerStatus& status, Enemy* enemy, float deltaTime)
{
    if (!enemy) {
        return;
    }

    const bool canPerformJustDodgeCounter =
        stateMachine.IsDodging() &&
        enemy->GetCanCountered();
    if (canPerformJustDodgeCounter) {
        player.GetGame()->OnPlayerCounter(movement.GetPlayerNum());

        enemy->ApplyBreak(deltaTime);
        enemy->FlipCanCountered();

        player.GetGame()->GetAudioSystem()->PlaySE("just_dodge_se");
        jewelGauge.Add(1);
        return;
    }

    ApplyDamageAndKnockBack(
        player,
        input,
        movement,
        stateMachine,
        combat,
        status,
        enemy->GetPos(),
        enemy->GetAttack());
}

void PlayerDamageHandler::ApplyFromActor(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerStateMachine& stateMachine,
    PlayerCombat& combat,
    PlayerStatus& status,
    const glm::vec3& damageSourcePosition,
    float damage)
{
    ApplyDamageAndKnockBack(
        player,
        input,
        movement,
        stateMachine,
        combat,
        status,
        damageSourcePosition,
        damage);
}
