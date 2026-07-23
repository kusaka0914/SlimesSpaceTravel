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

void PlayerDamageHandler::Apply(Player& player, PlayerInput& input, PlayerMovement& movement,
                                PlayerStateMachine& stateMachine, PlayerCombat& combat,
                                PlayerJewelGauge& jewelGauge, PlayerStatus& status, Enemy* enemy, float deltaTime)
{
    if (!enemy) {
        return;
    }

    if (stateMachine.IsDodging() && enemy->GetCanCountered()) {
        player.GetGame()->OnPlayerCounter(movement.GetPlayerNum());

        enemy->ApplyBreak(deltaTime, true);
        enemy->FlipCanCountered();

        player.GetGame()->GetAudioSystem()->PlaySE("just_dodge_se");
        jewelGauge.Add(1);
        return;
    }

    if (status.IsInvincible()) {
        return;
    }

    if (combat.GetCanSpecialAttack()) {
        combat.StartTiredLock(status, movement, 20.0f);
    }

    status.TakeDamage(enemy->GetAttack());
    movement.StartKnockBack(enemy->GetPos());
    movement.ClearStrongAttackDirectionOverride();
    stateMachine.ClearAttackDirectionTarget();
    player.SetShouldJudgeLanding(true);
    stateMachine.ChangeState(PlayerActionState::KnockedBack);

    player.GetGame()->OnPlayerApplyDamage(movement.GetPlayerNum());

    combat.CancelSpecialAttack();
    input.ClearAttackBuffer();
    input.SyncAttackButtonPrev();
}
