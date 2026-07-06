#include "actor/player/PlayerStatus.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerMovement.h"
#include "system/AudioSystem.h"

void PlayerStatus::ApplyDamage(PlayerModuleContext& context, Enemy* enemy, float deltaTime)
{
    Player& player = context.player;
    PlayerInput& input = context.input;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;

    if (combat.actionState == PlayerActionState::Dodging && enemy->GetCanCountered()) {
        player.GetGame()->OnPlayerCounter(movement.playerNum);
        enemy->ApplyBreak(deltaTime, true);
        enemy->FlipCanCountered();
        player.GetGame()->GetAudioSystem()->PlaySE("just_dodge_se");

        if (combat.jewelCount < 2) {
            combat.jewelCount++;
        }

        return;
    }

    if (invincibleTimer >= 0.0f) {
        return;
    }

    if (combat.canSpecialAttack) {
        isTired = true;
        combat.attackMoveLockRemaining = 20.0f;
        movement.dodgeCooldown = 20.0f;
        combat.attackCooldownRemaining = 20.0f;
    }

    hp -= enemy->GetAttack();

    movement.knockBackFrom = enemy->GetPos();
    damageTimer = defaultDamageTimer;
    invincibleTimer = defaultInvincibleTimer;

    combat.actionState = PlayerActionState::KnockedBack;

    player.GetGame()->OnPlayerApplyDamage(movement.playerNum);

    combat.canSpecialAttack = false;
    combat.specialChargingTimer = -1.0f;
    combat.continuousAttackingTimer = -1.0f;

    input.attackPressedPrev = input.attackPressed;
    input.wideAttackPressedPrev = input.wideAttackPressed;
    input.specialAttackPressedPrev = input.specialAttackPressed;
}

void PlayerStatus::Recover(PlayerModuleContext& context)
{
    PlayerCombat& combat = context.combat;

    combat.jewelCount--;
    hp += 1.0f;

    context.player.GetGame()->GetAudioSystem()->PlaySE("recover_se");

    if (hp >= maxHp) {
        hp = maxHp;
    }
}

void PlayerStatus::Die(PlayerModuleContext& context)
{
    context.player.GetGame()->OnPlayerDied();
}