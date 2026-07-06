#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"
#include "system/SceneSystem.h"

#include <cmath>
#include <glm/glm.hpp>

void PlayerStateMachine::Update(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;

    if (!player.GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    const glm::vec3 prevPos = player.GetPos();

    if (context.status.IsAlive()) {
        UpdateAlive(context, deltaTime);
        context.respawn.CheckFallRespawn(context, prevPos);
    } else {
        context.status.Die(context);
    }
}

void PlayerStateMachine::UpdateAlive(PlayerModuleContext& context, float deltaTime)
{
    PlayerInput& input = context.input;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;

    movement.UpdateWorldVec(context);
    movement.UpdateBoatRide(context);

    if (combat.jewelCount < 2 && combat.jewelTimer <= 0.0f) {
        StartJewelTimer(context);
    }

    switch (combat.actionState) {
    case PlayerActionState::Idle:
        UpdateIdle(context, deltaTime);
        break;
    case PlayerActionState::Dodging:
        UpdateDodging(context, deltaTime);
        break;
    case PlayerActionState::Attacking:
        UpdateAttacking(context, deltaTime);
        break;
    case PlayerActionState::Charging:
        UpdateCharging(context, deltaTime);
        break;
    case PlayerActionState::StrongAttacking:
        UpdateStrongAttacking(context, deltaTime);
        break;
    case PlayerActionState::KnockedBack:
        UpdateKnockedBack(context, deltaTime);
        break;
    }

    UpdateTimer(context, deltaTime);

    input.dodgePressedPrev = input.dodgePressed;
    input.attackPressedPrev = input.attackPressed;
    input.wideAttackPressedPrev = input.wideAttackPressed;
    input.specialAttackPressedPrev = input.specialAttackPressed;
    input.recoverPressedPrev = input.recoverPressed;
}

void PlayerStateMachine::UpdateIdle(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerInput& input = context.input;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;
    PlayerStatus& status = context.status;

    if (!player.GetIsActive()) {
        return;
    }

    const bool canStartCharging = !player.GetOnGround() && input.attackPressed && !combat.isStrongAttacked;
    if (canStartCharging) {
        combat.StartCharging(context, deltaTime);
        return;
    }

    if (combat.airAttackFloatingTimer <= 0.0f) {
        player.ModuleApplyGravity(deltaTime);
    }

    const bool canStartJumping = input.jumpPressed && player.GetOnGround();
    if (canStartJumping && combat.specialChargingTimer <= 0.0f && !combat.canSpecialAttack) {
        movement.StartJumping(context, deltaTime);
        return;
    }

    const bool canRecover =
        input.recoverPressed && !input.recoverPressedPrev && combat.jewelCount > 0 && status.hp != status.maxHp;

    if (canRecover) {
        status.Recover(context);
    }

    const bool isMoving = std::abs(input.moveForward) > 0.01f || std::abs(input.moveLeft) > 0.01f;
    if (isMoving && !status.isTired) {
        movement.ChangeFaceDir(context);
    }

    if (movement.CanWalk(combat) && combat.specialChargingTimer <= 0.0f && !combat.canSpecialAttack) {
        movement.UpdateWalk(context, deltaTime);
    }

    const bool isFalling = glm::dot(player.ModuleVelocity(), player.GetUpVec()) < 0.0f;
    if (isFalling) {
        player.ModuleShouldJudgeLanding() = true;
    }

    const bool canSpecialAttack =
        input.specialAttackPressed && input.attackPressed && !input.attackPressedPrev && combat.jewelCount >= 2;

    if (canSpecialAttack) {
        combat.StartSpecialAttackCharging(context);
        return;
    }

    if (input.wideAttackPressed && !input.wideAttackPressedPrev && status.isTired) {
        ReduceTired(context);
        return;
    }

    const bool canContinuousAttacking =
        input.specialAttackPressed && input.wideAttackPressed && !input.wideAttackPressedPrev && combat.jewelCount >= 1;

    if (canContinuousAttacking) {
        combat.StartContinuousAttacking(context);
        return;
    }

    if (combat.specialChargingTimer >= 0.0f || combat.canSpecialAttack) {
        UpdateSpecialAttackCharging(context, deltaTime);
    }

    if (combat.continuousAttackingTimer >= 0.0f) {
        UpdateContinuousAttacking(context, deltaTime);
        return;
    }

    const bool canStartDodging = movement.dodgeCooldown <= 0.0f && combat.attackDodgeLockRemaining <= 0.0f &&
                                 !movement.isDodged && input.dodgePressed && !input.dodgePressedPrev;

    if (canStartDodging) {
        movement.StartDodging(context);
        return;
    }

    const bool canStartAttacking =
        combat.attackCooldownRemaining <= 0.0f &&
        ((input.attackPressed || input.wideAttackPressed) && !input.attackPressedPrev && !input.wideAttackPressedPrev);

    if (canStartAttacking && combat.specialChargingTimer <= 0.0f && !combat.canSpecialAttack) {
        combat.StartAttacking(context, deltaTime);
        return;
    }
}

void PlayerStateMachine::UpdateDodging(PlayerModuleContext& context, float deltaTime)
{
    PlayerMovement& movement = context.movement;

    movement.MoveDuringDodging(context, deltaTime);

    movement.dodgeTimer -= deltaTime;
    if (movement.dodgeTimer <= 0.0f) {
        StartIdle(context);
    }
}

void PlayerStateMachine::UpdateAttacking(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;

    if (player.GetOnGround()) {
        movement.MoveDuringAttacking(context, deltaTime);
    }

    if (movement.CanWalk(combat)) {
        movement.UpdateWalk(context, deltaTime);
    }

    combat.attackMotionTimer -= deltaTime;
    if (combat.attackMotionTimer <= 0.0f) {
        StartIdle(context);
    }
}

void PlayerStateMachine::UpdateCharging(PlayerModuleContext& context, float deltaTime)
{
    PlayerInput& input = context.input;
    PlayerCombat& combat = context.combat;
    PlayerMovement& movement = context.movement;

    const bool isAttackBtnReleased = !input.attackPressed;
    if (isAttackBtnReleased) {
        combat.StartStrongAttacking(context, deltaTime);
        return;
    }

    if (combat.attackPressTimer < 0.0f) {
        return;
    }

    combat.attackPressTimer -= deltaTime;
    if (combat.attackPressTimer >= 0.0f) {
        movement.MoveDuringCharging(context, deltaTime);
        return;
    }

    combat.FinishCharging(context);
}

void PlayerStateMachine::UpdateStrongAttacking(PlayerModuleContext& context, float deltaTime)
{
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;

    movement.MoveDuringStrongAttacking(context, deltaTime);

    combat.strongAttackTimer -= deltaTime;
    if (combat.strongAttackTimer >= 0.0f) {
        return;
    }

    StartIdle(context);

    if (!combat.isCharged) {
        return;
    }

    if (!combat.isStrongAttackHit) {
        combat.Attack(context, deltaTime);
    }

    if (combat.isStrongAttackHit) {
        combat.isStrongAttackHit = false;
        context.player.GetGame()->OnStrongAttacked(movement.playerNum);
    }
}

void PlayerStateMachine::UpdateKnockedBack(PlayerModuleContext& context, float deltaTime)
{
    PlayerMovement& movement = context.movement;
    PlayerStatus& status = context.status;

    movement.MoveDuringKnockBack(context, deltaTime);

    status.damageTimer -= deltaTime;
    if (status.damageTimer <= 0.0f) {
        StartIdle(context);
    }
}

void PlayerStateMachine::UpdateSpecialAttackCharging(PlayerModuleContext& context, float deltaTime)
{
    PlayerInput& input = context.input;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;

    const float specialChargingTimerPrev = combat.specialChargingTimer;

    combat.specialChargingTimer -= deltaTime;

    if (specialChargingTimerPrev >= 2.0f && combat.specialChargingTimer <= 2.0f) {
        context.player.GetGame()->VibrateControllerForPlayer(movement.playerNum, 10000, 0, 1000);
        combat.jewelCount--;
        context.player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 1.0f && combat.specialChargingTimer <= 1.0f) {
        context.player.GetGame()->VibrateControllerForPlayer(movement.playerNum, 20000, 0, 1000);
        combat.jewelCount--;
        context.player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 0.0f && combat.specialChargingTimer <= 0.0f) {
        context.player.GetGame()->VibrateControllerForPlayer(movement.playerNum, 30000, 0, 1000);
        context.player.GetGame()->GetAudioSystem()->PlaySE("charged_se");
    }

    if (combat.specialChargingTimer <= 0.0f) {
        combat.canSpecialAttack = true;
    }

    if (combat.specialChargingTimer <= 0.0f && input.attackPressed && !input.attackPressedPrev) {
        combat.SpecialAttack(context, deltaTime);
    }

    if (input.attackPressed && !input.attackPressedPrev) {
        combat.specialChargingTimer = -1.0f;
    }
}

void PlayerStateMachine::UpdateContinuousAttacking(PlayerModuleContext& context, float deltaTime)
{
    PlayerCombat& combat = context.combat;

    combat.attackKind = PlayerAttackKind::Wide;
    combat.attack = combat.wideAttack / 2.0f;
    combat.attackRange = combat.wideAttackRange;
    combat.attackAngle = combat.wideAttackAngle;

    combat.continuousAttackingTimer -= deltaTime;
    combat.continuousAttackingCooldown -= deltaTime;

    if (combat.continuousAttackingCooldown <= 0.0f) {
        combat.continuousAttackingCooldown = 0.25f;
        combat.Attack(context, deltaTime);
        combat.attackMoveLockRemaining = 0.0f;
    }
}

void PlayerStateMachine::UpdateTimer(PlayerModuleContext& context, float deltaTime)
{
    PlayerInput& input = context.input;
    PlayerMovement& movement = context.movement;
    PlayerCombat& combat = context.combat;
    PlayerStatus& status = context.status;

    if (combat.airAttackFloatingTimer > 0.0f) {
        combat.airAttackFloatingTimer -= deltaTime;
    }

    if (movement.dodgeCooldown > 0.0f) {
        movement.dodgeCooldown -= deltaTime;
    }

    if (combat.jewelTimer >= 0.0f) {
        UpdateJewelTimer(context, deltaTime);
    }

    if (combat.attackCooldownRemaining >= 0.0f) {
        combat.attackCooldownRemaining -= deltaTime;
    }

    if (combat.attackMoveLockRemaining > 0.0f) {
        combat.attackMoveLockRemaining -= deltaTime;
        if (status.isTired && combat.attackMoveLockRemaining <= 0.0f) {
            status.isTired = false;
        }
    }

    if (combat.attackDodgeLockRemaining > 0.0f) {
        combat.attackDodgeLockRemaining -= deltaTime;
    }

    if (status.invincibleTimer >= 0.0f) {
        status.invincibleTimer -= deltaTime;
    }

    if (combat.rayCastTimer >= 0.0f) {
        combat.rayCastTimer -= deltaTime;
    }

    if (input.inputAvailableTimer >= 0.0f) {
        input.inputAvailableTimer -= deltaTime;
    }

    if (combat.comboKeepTimer > 0.0f) {
        UpdateComboKeepTimer(context, deltaTime);
    }
}

void PlayerStateMachine::UpdateJewelTimer(PlayerModuleContext& context, float deltaTime)
{
    PlayerCombat& combat = context.combat;

    combat.jewelTimer -= deltaTime;
    if (combat.jewelTimer >= 0.0f) {
        return;
    }

    if (combat.jewelCount < 2) {
        combat.jewelCount++;
    }
}

void PlayerStateMachine::UpdateComboKeepTimer(PlayerModuleContext& context, float deltaTime)
{
    PlayerCombat& combat = context.combat;

    combat.comboKeepTimer -= deltaTime;
    if (combat.comboKeepTimer >= 0.0f) {
        return;
    }

    combat.attackComboIndex = 0;
}

void PlayerStateMachine::StartIdle(PlayerModuleContext& context)
{
    context.combat.actionState = PlayerActionState::Idle;
}

void PlayerStateMachine::StartJewelTimer(PlayerModuleContext& context)
{
    context.combat.jewelTimer = 30.0f;
}

void PlayerStateMachine::StartTired(PlayerModuleContext& context, float lockTime)
{
    context.status.isTired = true;
    context.combat.attackMoveLockRemaining = lockTime;
    context.movement.dodgeCooldown = lockTime;
    context.combat.attackCooldownRemaining = lockTime;
}

void PlayerStateMachine::ReduceTired(PlayerModuleContext& context)
{
    constexpr float reduceTime = 0.8f;

    context.combat.attackMoveLockRemaining -= reduceTime;
    context.movement.dodgeCooldown -= reduceTime;
    context.combat.attackCooldownRemaining -= reduceTime;

    if (context.combat.attackMoveLockRemaining <= 0.0f) {
        context.status.isTired = false;
    }
}