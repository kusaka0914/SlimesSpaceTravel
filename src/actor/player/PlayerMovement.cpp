#include "actor/player/PlayerMovement.h"

#include "actor/Boat.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>

bool PlayerMovement::CanWalk(const PlayerCombat& combat) const
{
    return combat.CanMoveDuringAttack();
}

bool PlayerMovement::CanDodge(const PlayerCombat& combat) const
{
    return mDodgeCooldown <= 0.0f && combat.CanDodgeDuringAttack() && !mIsDodged;
}

void PlayerMovement::UpdateWorldVec(Player& player, const PlayerInput& input)
{
    const glm::vec3 upVec = player.GetUpVec();
    glm::vec3 projectedForward = mForwardVec - glm::dot(mForwardVec, upVec) * upVec;

    if (glm::length(projectedForward) < 1e-6f) {
        projectedForward = glm::cross(glm::vec3(1, 0, 0), upVec);
        if (glm::length(projectedForward) < 1e-6f) {
            projectedForward = glm::cross(glm::vec3(0, 1, 0), upVec);
        }
    }

    projectedForward = glm::normalize(projectedForward);
    glm::vec3 baseLeft = glm::normalize(glm::cross(upVec, projectedForward));

    mForwardVec =
        glm::normalize(projectedForward * std::cos(input.GetCameraYaw()) - baseLeft * std::sin(input.GetCameraYaw()));
    mLeftVec = glm::normalize(glm::cross(upVec, mForwardVec));
}

void PlayerMovement::UpdateWalk(Player& player, const PlayerInput& input, float deltaTime)
{
    glm::vec3 moveDelta = mForwardVec * input.GetMoveForward() * mMoveSpeed * deltaTime +
                          mLeftVec * input.GetMoveLeft() * mMoveSpeed * deltaTime;

    glm::vec3 desiredPos = player.GetPos() + moveDelta;
    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);

    player.SetPos(desiredPos);
}

void PlayerMovement::UpdateBoatRide(Player& player, PlayerRespawn& respawn)
{
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
            FollowMovingBoat(player, boat);
            return;
        }

        if (IsTouchingBoat(player, boat)) {
            StartRidingBoat(player, boat);
            return;
        }
    }
}

void PlayerMovement::ChangeFaceDir(Player& player, const PlayerInput& input)
{
    glm::vec3 moveDir = glm::normalize(mForwardVec * input.GetMoveForward() + mLeftVec * input.GetMoveLeft());

    player.SetFacingForwardVec(moveDir);
    player.SetFacingYaw(player.GetGame()->GetMathUtils()->GetYawFromDirection(player.GetUpVec(), moveDir) +
                        3.14159265f);
}

void PlayerMovement::UpdateFacingForwardVec(Player& player)
{
    glm::vec3 up = player.GetUpVec();
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = player.GetFacingForwardVec();
    forward = forward - up * glm::dot(forward, up);

    if (glm::length(forward) < 1e-6f) {
        forward = mForwardVec;
        forward = forward - up * glm::dot(forward, up);
    }

    if (glm::length(forward) < 1e-6f) {
        return;
    }

    player.SetFacingForwardVec(glm::normalize(forward));
    player.SetFacingYaw(player.GetGame()->GetMathUtils()->GetYawFromDirection(up, player.GetFacingForwardVec()) +
                        3.14159265f);
}

void PlayerMovement::MoveDuringDodging(Player& player, const PlayerCombat& combat, float deltaTime)
{
    float dodgeSpeed =
        player.GetOnGround() ? (mDodgeDistance / mDodgeDuration) : (mDodgeDistance / (mDodgeDuration * 4.0f));

    if (combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        dodgeSpeed /= 4.0f;
    }

    const glm::vec3 moveDelta = mDodgeDir * dodgeSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    if (player.GetOnGround()) {
        SnapToGround(player, 0.5f, 1.0f);
    }
}

void PlayerMovement::MoveDuringAttacking(Player& player, const PlayerCombat& combat, float deltaTime)
{
    glm::vec3 moveDelta = player.GetFacingForwardVec() * combat.GetAttackSpeed() * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(player);
}

void PlayerMovement::MoveDuringCharging(Player& player, float deltaTime)
{
    const glm::vec3 moveDelta = -player.GetFacingForwardVec() * mChargeMoveSpeed * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(player);
}

void PlayerMovement::MoveDuringStrongAttacking(Player& player, const PlayerCombat& combat, float deltaTime)
{
    const glm::vec3 moveDelta = player.GetFacingForwardVec() * combat.GetStrongAttackSpeed() * deltaTime;
    glm::vec3 desiredPos = player.GetPos() + moveDelta;

    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);
    player.SetPos(desiredPos);

    UpdateFacingForwardVec(player);
}

void PlayerMovement::MoveDuringKnockBack(Player& player, float deltaTime)
{
    glm::vec3 toPlayer = glm::normalize(player.GetPos() - mKnockBackFrom);
    player.SetPos(player.GetPos() + toPlayer * mKnockBackSpeed * deltaTime);
}

void PlayerMovement::FollowMovingBoat(Player& player, Boat* boat)
{
    player.SetPos(boat->GetPos());
    player.SetIsActive(false);
}

bool PlayerMovement::IsTouchingBoat(const Player& player, Boat* boat) const
{
    constexpr float boatTouchRadius = 0.9f;
    const float distToBoat = glm::length(player.GetPos() - boat->GetPos());

    return distToBoat <= boatTouchRadius;
}

void PlayerMovement::StartDodging(Player& player, const PlayerInput& input, PlayerCombat& combat, PlayerStatus& status)
{
    combat.StartDodging();

    if (input.GetMoveForward() != 0.0f || input.GetMoveLeft() != 0.0f) {
        mDodgeDir = player.GetFacingForwardVec();
    } else {
        mDodgeDir = -player.GetFacingForwardVec();
    }

    mDodgeTimer = player.GetOnGround() ? mDodgeDuration : mDodgeDuration * 4.0f;
    mDodgeCooldown = mDodgeCooldownTime;
    status.StartInvincible(mDodgeDuration);

    player.SetVelocity(glm::vec3(0.0f));
    player.GetGame()->GetAudioSystem()->PlaySE("dodge_se");

    mIsDodged = true;
}

void PlayerMovement::StartJumping(Player& player, float deltaTime)
{
    constexpr float jumpPower = 6.0f;

    player.AddVelocity(player.GetUpVec() * jumpPower);
    player.SetPos(player.GetPos() + player.GetVelocity() * deltaTime);

    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);

    player.GetGame()->GetAudioSystem()->PlaySE("jump_se");
}

void PlayerMovement::StartRidingBoat(Player& player, Boat* boat)
{
    if (!player.GetIsActive()) {
        return;
    }

    boat->StartTravel();
    player.SetIsActive(false);
}

void PlayerMovement::OnBoatArrived(Player& player, PlayerRespawn& respawn, Boat* boat)
{
    player.SetCurrentPlanet(boat->GetDestPlanet());
    player.SetPos(boat->GetDestPos());

    respawn.SetRestartPos(player.GetPos());
    respawn.SetRestartPlanetIndex(mCurrentPlanetNum);

    player.SetVelocity(glm::vec3(0.0f));
    player.SetIsActive(true);

    player.RefreshFallbackUpVec();
}

void PlayerMovement::OnLanded(Player& player, PlayerCombat& combat)
{
    mIsDodged = false;
    combat.OnLanded();
    player.GetGame()->OnLanded();
}

void PlayerMovement::OnUpVecUpdateFailed(Player& player, PlayerCombat& combat)
{
    if (combat.GetRayCastTimer() > 0.0f) {
        return;
    }

    if (!player.GetCurrentPlanet()) {
        return;
    }

    if (player.GetCurrentPlanet()->GetPlanetShape() == Planet::PlanetShape::Normal) {
        return;
    }

    player.RefreshFallbackUpVec();
    player.SetVelocity(glm::vec3(0.0f));
    combat.ResetRayCastTimer();
}

void PlayerMovement::OnCastSucceeded(PlayerCombat& combat)
{
    combat.ResetRayCastTimer();
}

void PlayerMovement::SnapToGround(Player& player, float upOffset, float downLength)
{
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
    player.SetOnGround(true);
    player.SetVelocity(glm::vec3(0.0f));
}

void PlayerMovement::ReduceDodgeCooldown(float deltaTime)
{
    if (mDodgeCooldown > 0.0f) {
        mDodgeCooldown -= deltaTime;
    }
}