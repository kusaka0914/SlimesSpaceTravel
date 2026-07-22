#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerBoatRide.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStatus.h"
#include "system/SceneSystem.h"

#include <glm/glm.hpp>

void PlayerStateMachine::Update(Player& player, PlayerInput& input, PlayerMovement& movement,
                                PlayerGrounding& grounding, PlayerBoatRide& boatRide, PlayerCombat& combat,
                                PlayerJewelGauge& jewelGauge, PlayerStatus& status, PlayerRespawn& respawn,
                                float deltaTime)
{
    if (!player.GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    const glm::vec3 prevPos = player.GetPos();

    if (status.IsAlive()) {
        UpdateAlive(player, input, movement, grounding, boatRide, combat, jewelGauge, status, respawn, deltaTime);
        respawn.CheckFallRespawn(player, *this, combat, status, prevPos);
    } else {
        status.Die(*player.GetGame());
    }
}

void PlayerStateMachine::UpdateAlive(Player& player, PlayerInput& input, PlayerMovement& movement,
                                     PlayerGrounding& grounding, PlayerBoatRide& boatRide, PlayerCombat& combat,
                                     PlayerJewelGauge& jewelGauge, PlayerStatus& status, PlayerRespawn& respawn,
                                     float deltaTime)
{
    movement.UpdateCameraRelativeMovementDirections(player, input);
    boatRide.Update(player, movement, respawn);

    if (jewelGauge.ShouldStartRecoverTimer()) {
        StartJewelTimer(jewelGauge);
    }

    switch (mActionState) {
    case PlayerActionState::Idle:
        UpdateIdle(player, input, movement, combat, jewelGauge, status, deltaTime);
        break;
    case PlayerActionState::Dodging:
        UpdateDodging(player, movement, grounding, combat, deltaTime);
        break;
    case PlayerActionState::Attacking:
        UpdateAttacking(player, input, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::Charging:
        UpdateCharging(player, input, movement, combat, deltaTime);
        break;
    case PlayerActionState::StrongAttacking:
        UpdateStrongAttacking(player, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::KnockedBack:
        UpdateKnockedBack(player, movement, combat, status, deltaTime);
        break;
    }

    UpdateTimer(input, movement, grounding, combat, jewelGauge, status, deltaTime);
    input.EndFrame();
}

void PlayerStateMachine::UpdateTimer(PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding,
                                     PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status,
                                     float deltaTime)
{
    combat.UpdateAirAttackFloatingTimer(deltaTime);
    movement.UpdateDodgeCooldown(deltaTime);

    if (jewelGauge.GetRecoverTimer() >= 0.0f) {
        jewelGauge.UpdateRecoverTimer(deltaTime);
    }

    combat.UpdateAttackCooldown(deltaTime);
    combat.UpdateAttackMoveLock(status, deltaTime);
    combat.UpdateAttackDodgeLock(deltaTime);
    status.UpdateInvincibleTimer(deltaTime);
    grounding.UpdateRayCastTimer(deltaTime);
    input.UpdateInputAvailableTimer(deltaTime);

    if (combat.GetComboKeepTimer() > 0.0f) {
        combat.UpdateComboKeepTimer(deltaTime);
    }
}

bool PlayerStateMachine::IsAttackingState() const
{
    return mActionState == PlayerActionState::Attacking || mActionState == PlayerActionState::Charging ||
           mActionState == PlayerActionState::StrongAttacking;
}

void PlayerStateMachine::StartIdle()
{
    ChangeState(PlayerActionState::Idle);
}

void PlayerStateMachine::StartJewelTimer(PlayerJewelGauge& jewelGauge)
{
    jewelGauge.StartRecoverTimer(30.0f);
}

void PlayerStateMachine::StartTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat,
                                    float lockTime)
{
    combat.StartTiredLock(status, movement, lockTime);
}

void PlayerStateMachine::ReduceTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat)
{
    constexpr float reduceTime = 0.8f;
    combat.ReduceTiredLock(status, movement, reduceTime);
}
