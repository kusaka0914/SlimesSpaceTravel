#include "actor/player/PlayerMovement.h"

#include "actor/Boat.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerModuleContext.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>

bool PlayerMovement::CanWalk(const PlayerCombat& combat) const
{
    return combat.attackMoveLockRemaining <= 0.0f && !combat.isAirAttacking;
}

void PlayerMovement::UpdateWorldVec(PlayerModuleContext& context)
{
    Player& player = context.player;
    PlayerInput& input = context.input;

    const glm::vec3 upVec = player.GetUpVec();
    glm::vec3 projectedForward = forwardVec - glm::dot(forwardVec, upVec) * upVec;

    if (glm::length(projectedForward) < 1e-6f) {
        projectedForward = glm::cross(glm::vec3(1, 0, 0), upVec);
        if (glm::length(projectedForward) < 1e-6f) {
            projectedForward = glm::cross(glm::vec3(0, 1, 0), upVec);
        }
    }

    projectedForward = glm::normalize(projectedForward);
    glm::vec3 baseLeft = glm::normalize(glm::cross(upVec, projectedForward));

    forwardVec = glm::normalize(projectedForward * std::cos(input.cameraYaw) - baseLeft * std::sin(input.cameraYaw));
    leftVec = glm::normalize(glm::cross(upVec, forwardVec));
}

void PlayerMovement::UpdateWalk(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerInput& input = context.input;

    glm::vec3 moveDelta =
        forwardVec * input.moveForward * moveSpeed * deltaTime + leftVec * input.moveLeft * moveSpeed * deltaTime;

    glm::vec3 desiredPos = player.GetPos() + moveDelta;
    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);

    player.SetPos(desiredPos);
}

void PlayerMovement::UpdateBoatRide(PlayerModuleContext& context)
{
    Player& player = context.player;

    if (!player.GetCurrentPlanet()) {
        return;
    }

    const std::vector<Boat*>& boats = player.GetCurrentPlanet()->GetBoats();
    if (boats.empty()) {
        return;
    }

    for (Boat* boat : boats) {
        if (!boat->GetIsActive()) {
            continue;
        }

        if (boat->GetIsMoving()) {
            FollowMovingBoat(context, boat);
            return;
        }

        if (IsTouchingBoat(context, boat)) {
            StartRidingBoat(context, boat);
            return;
        }
    }
}

void PlayerMovement::ChangeFaceDir(PlayerModuleContext& context)
{
    Player& player = context.player;
    PlayerInput& input = context.input;

    glm::vec3 moveDir = glm::normalize(forwardVec * input.moveForward + leftVec * input.moveLeft);

    player.ModuleFacingForwardVec() = moveDir;
    player.SetFacingYaw(player.GetGame()->GetMathUtils()->GetYawFromDirection(player.GetUpVec(), moveDir) +
                        3.14159265f);
}

void PlayerMovement::UpdateFacingForwardVec(PlayerModuleContext& context)
{
    Player& player = context.player;

    glm::vec3 up = player.GetUpVec();
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = player.GetFacingForwardVec();
    forward = forward - up * glm::dot(forward, up);

    if (glm::length(forward) < 1e-6f) {
        forward = forwardVec;
        forward = forward - up * glm::dot(forward, up);
    }

    if (glm::length(forward) < 1e-6f) {
        return;
    }

    player.ModuleFacingForwardVec() = glm::normalize(forward);
    player.SetFacingYaw(player.GetGame()->GetMathUtils()->GetYawFromDirection(up, player.GetFacingForwardVec()) +
                        3.14159265f);
}

void PlayerMovement::MoveDuringDodging(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerCombat& combat = context.combat;

    float dodgeSpeed =
        player.GetOnGround() ? (dodgeDistance / dodgeDuration) : (dodgeDistance / (dodgeDuration * 4.0f));

    if (combat.specialChargingTimer >= 0.0f || combat.canSpecialAttack) {
        dodgeSpeed /= 4.0f;
    }

    const glm::vec3 moveDelta = dodgeDir * dodgeSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    if (player.GetOnGround()) {
        SnapToGround(context, 0.5f, 1.0f);
    }
}

void PlayerMovement::MoveDuringAttacking(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerCombat& combat = context.combat;

    glm::vec3 moveDelta = player.GetFacingForwardVec() * combat.attackSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(context);
}

void PlayerMovement::MoveDuringCharging(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;

    const glm::vec3 moveDelta = -player.GetFacingForwardVec() * chargeMoveSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(context);
}

void PlayerMovement::MoveDuringStrongAttacking(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;
    PlayerCombat& combat = context.combat;

    const glm::vec3 moveDelta = player.GetFacingForwardVec() * combat.strongAttackSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(context);
}

void PlayerMovement::MoveDuringKnockBack(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;

    glm::vec3 toPlayer = glm::normalize(player.GetPos() - knockBackFrom);
    player.SetPos(player.GetPos() + toPlayer * knockBackSpeed * deltaTime);
}

void PlayerMovement::FollowMovingBoat(PlayerModuleContext& context, Boat* boat)
{
    Player& player = context.player;

    player.SetPos(boat->GetPos());
    player.SetIsActive(false);
}

bool PlayerMovement::IsTouchingBoat(PlayerModuleContext& context, Boat* boat)
{
    const Player& player = context.player;

    constexpr float boatTouchRadius = 0.9f;
    const float distToBoat = glm::length(player.GetPos() - boat->GetPos());

    return distToBoat <= boatTouchRadius;
}

void PlayerMovement::StartDodging(PlayerModuleContext& context)
{
    Player& player = context.player;
    PlayerInput& input = context.input;
    PlayerCombat& combat = context.combat;
    PlayerStatus& status = context.status;

    combat.actionState = PlayerActionState::Dodging;

    if (input.moveForward != 0.0f || input.moveLeft != 0.0f) {
        dodgeDir = player.GetFacingForwardVec();
    } else {
        dodgeDir = -player.GetFacingForwardVec();
    }

    dodgeTimer = player.GetOnGround() ? dodgeDuration : dodgeDuration * 4.0f;
    dodgeCooldown = dodgeCooldownTime;
    status.invincibleTimer = dodgeDuration;

    player.SetVelocity(glm::vec3(0.0f));
    player.GetGame()->GetAudioSystem()->PlaySE("dodge_se");

    isDodged = true;
}

void PlayerMovement::StartJumping(PlayerModuleContext& context, float deltaTime)
{
    Player& player = context.player;

    constexpr float jumpPower = 6.0f;

    player.ModuleVelocity() += player.GetUpVec() * jumpPower;
    player.SetPos(player.GetPos() + player.ModuleVelocity() * deltaTime);

    player.ModuleOnGround() = false;
    player.ModuleShouldJudgeLanding() = false;

    player.GetGame()->GetAudioSystem()->PlaySE("jump_se");
}

void PlayerMovement::StartRidingBoat(PlayerModuleContext& context, Boat* boat)
{
    Player& player = context.player;

    if (!player.GetIsActive()) {
        return;
    }

    boat->StartTravel();
    player.SetIsActive(false);
}

void PlayerMovement::OnBoatArrived(PlayerModuleContext& context, Boat* boat)
{
    Player& player = context.player;

    player.SetCurrentPlanet(boat->GetDestPlanet());
    player.SetPos(boat->GetDestPos());

    context.respawn.restartPos = player.GetPos();
    context.respawn.restartPlanetIndex = currentPlanetNum;

    player.SetVelocity(glm::vec3(0.0f));
    player.SetIsActive(true);

    player.ModuleUpdateFallbackUpVec();
}

void PlayerMovement::OnLanded(PlayerModuleContext& context)
{
    isDodged = false;

    context.combat.isStrongAttacked = false;
    context.combat.isCharged = false;
    context.combat.isAirAttacking = false;

    context.player.GetGame()->OnLanded();
}

void PlayerMovement::OnUpVecUpdateFailed(PlayerModuleContext& context)
{
    Player& player = context.player;

    if (context.combat.rayCastTimer > 0.0f) {
        return;
    }

    if (!player.GetCurrentPlanet()) {
        return;
    }

    if (player.GetCurrentPlanet()->GetPlanetShape() == Planet::PlanetShape::Normal) {
        return;
    }

    player.ModuleUpdateFallbackUpVec();
    player.SetVelocity(glm::vec3(0.0f));
    context.combat.rayCastTimer = 0.5f;
}

void PlayerMovement::OnCastSucceeded(PlayerModuleContext& context)
{
    context.combat.rayCastTimer = 0.5f;
}

void PlayerMovement::SnapToGround(PlayerModuleContext& context, float upOffset, float downLength)
{
    Player& player = context.player;

    if (glm::length(player.GetUpVec()) < 1e-6f) {
        return;
    }

    glm::vec3 up = glm::normalize(player.GetUpVec());

    glm::vec3 from = player.GetPos() + up * upOffset;
    glm::vec3 to = player.GetPos() - up * downLength;

    btCollisionWorld::ClosestRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

    auto* bulletWorld = player.GetGame()->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return;
    }

    bulletWorld->rayTest(cb.m_rayFromWorld, cb.m_rayToWorld, cb);

    if (!cb.hasHit()) {
        return;
    }

    glm::vec3 hitPos(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());

    player.SetPos(hitPos);
    player.ModuleOnGround() = true;
    player.SetVelocity(glm::vec3(0.0f));
}