#include "actor/player/PlayerMovement.h"

#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

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
    const glm::vec3 moveDelta = mForwardVec * input.GetMoveForward() * mMoveSpeed * deltaTime +
                                mLeftVec * input.GetMoveLeft() * mMoveSpeed * deltaTime;

    glm::vec3 desiredPos = player.GetPos() + moveDelta;
    desiredPos = player.GetGame()->GetPhysicsSystem()->CheckCollision(&player, moveDelta, desiredPos);

    player.SetPos(desiredPos);
}

void PlayerMovement::ChangeFaceDir(Player& player, const PlayerInput& input)
{
    const glm::vec3 moveDir = glm::normalize(mForwardVec * input.GetMoveForward() + mLeftVec * input.GetMoveLeft());

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

void PlayerMovement::MoveDuringDodging(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding,
                                       float deltaTime)
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
        grounding.SnapToGround(player, 0.5f, 1.0f);
    }
}

void PlayerMovement::MoveDuringAttacking(Player& player, const PlayerCombat& combat, float deltaTime)
{
    const glm::vec3 moveDelta = player.GetFacingForwardVec() * combat.GetAttackSpeed() * deltaTime;
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
    const glm::vec3 toPlayer = glm::normalize(player.GetPos() - mKnockBackFrom);
    player.SetPos(player.GetPos() + toPlayer * mKnockBackSpeed * deltaTime);
}

void PlayerMovement::StartDodging(Player& player, const PlayerInput& input, PlayerStatus& status)
{
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

void PlayerMovement::ReduceDodgeCooldown(float deltaTime)
{
    if (mDodgeCooldown > 0.0f) {
        mDodgeCooldown -= deltaTime;
    }
}
