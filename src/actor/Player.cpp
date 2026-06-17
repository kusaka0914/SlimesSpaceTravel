#include "Player.h"
#include "Game.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <iostream>

Player::Player(Game* game)
    : CharacterActor(game),
      mActionState(ActionState::Idle),
      mAttackKind(AttackKind::Normal),
      mDodgePressed(false),
      mDodgePressedPrev(false),
      mJumpPressed(false),
      mAttackPressed(false),
      mAttackPressedPrev(false),
      mWideAttackPressed(false),
      mWideAttackPressedPrev(false),
      mSpecialAttackPressed(false),
      mSpecialAttackPressedPrev(false),
      mRecoverPressed(false),
      mRecoverPressedPrev(false),
      mIsDodged(true),
      mIsStrongAttackHit(false),
      mIsStrongAttacked(false),
      mIsCharged(false),
      mCurrentPlanetNum(0),
      mAttackComboIndex(0),
      mRestartPlanetIndex(0),
      mPlayerNum(1),
      mJewelCount(2),
      mCameraYaw(0.0f),
      mMoveForward(0.0f),
      mMoveLeft(0.0f),
      mAttackStartHeight(0.0f),
      mDodgeTimer(0.0f),
      mDodgeDuration(0.1f),
      mDodgeCooldown(0.0f),
      mDodgeCooldownTime(0.3f),
      mDodgeDistance(3.0f),
      mDodgeStartHeight(0.0f),
      mMoveSpeed(10.2f),
      mChargeMoveSpeed(6.0f),
      mCameraStickX(0.0f),
      mCameraStickY(0.0f),
      mAttack(10.0f),
      mAttackSpeed(5.0f),
      mHp(100.0f),
      mMaxHp(100.0f),
      mDamageTimer(0.0f),
      mDefaultDamageTimer(1.0f),
      mAttackCooldownRemaining(0.0f),
      mAttackCooldown(0.3f),
      mLastAttackCooldown(1.0f),
      mAttackMoveLockRemaining(-1.0f),
      mAttackDodgeLockRemaining(0.0f),
      mAttackMotionTimer(-1.0f),
      mDefaultAttackMotionTimer(0.3f),
      mJewelTimer(-1.0f),
      mSpecialAttackCooldown(30.0f),
      mAttackPressTimer(-1.0f),
      mStrongAttackTimer(-1.0f),
      mDefaultStrongAttackTimer(0.06f),
      mComboKeepTimer(-1.0f),
      mInvincibleTimer(-1.0f),
      mDefaultInvincibleTimer(2.0f),
      mAttackRange(2.8f),
      mAttackAngle(0.8f),
      mNormalAttackRange(2.8f),
      mNormalAttackAngle(0.8f),
      mNormalAttack(10.0f),
      mWideAttackRange(2.8f),
      mWideAttackAngle(-0.2f),
      mWideAttack(5.0f),
      mStrongAttackRange(6.0f),
      mStrongAttack(50.0f),
      mStrongAttackSpeed(100.0f),
      mRayCastTimer(0.5f),
      mInputAvailableTimer(-1.0f),
      mForwardVec(0.0f, 0.0f, 1.0f),
      mLeftVec(-1.0f, 0.0f, 0.0f),
      mKnockBackFrom(0.0f),
      mRestartPos(0.0f),
      mDodgeDir(0.0f),
      mRayCasts(),
      mTalkableNPC(nullptr),
      mAirAttackFloatingTimer(-1.0f),
      mKnockBackSpeed(0.0f),
      mContinuousAttackingTimer(-1.0f),
      mContinuousAttackingCooldown(-1.0f),
      mSpecialChargingTimer(-1.0f),
      mCanSpecialAttack(false),
      mIsTired(false)
{
}

void Player::Initialize()
{
    mRestartPlanetIndex = mCurrentPlanetNum;
    mRestartPos = mPos;
}

void Player::ProcessActor()
{
    if (mInputAvailableTimer >= 0.0f)
        return;

    ProcessGameController();
    ProcessKeyboard();
}

void Player::ProcessGameController()
{
    if (!mGame->IsGameControllerConnected() || mPlayerNum != 1)
        return;

    SDL_GameController* sdlController = mGame->GetSdlController();

    constexpr float deadZone = 0.25f;
    constexpr float scale =
        1.0f / 32767.0f; // SDL_GameControllerGetAxisの範囲が32767までで、scaleをかけて1.0f以内に抑えるため

    mMoveForward = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY) * scale;
    mMoveLeft = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX) * scale;

    if (std::abs(mMoveForward) < deadZone)
        mMoveForward = 0.0f;
    if (std::abs(mMoveLeft) < deadZone)
        mMoveLeft = 0.0f;

    mJumpPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_A);
    mAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_X);
    mWideAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_Y);
    mDodgePressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_B);
    mSpecialAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mRecoverPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
}

void Player::ProcessKeyboard()
{
    if (mGame->IsGameControllerConnected())
        return;

    GLFWwindow* window = mGame->GetWindow();
    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        mMoveForward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        mMoveForward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        mMoveLeft -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        mMoveLeft += 1.0f;

    constexpr float cameraKeySpeed = 0.02f;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        mCameraYaw += cameraKeySpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        mCameraYaw -= cameraKeySpeed;
    }

    glm::vec2 moveInput(mMoveLeft, mMoveForward);

    if (glm::length(moveInput) > 1.0f) {
        moveInput = glm::normalize(moveInput);
    }

    mMoveLeft = moveInput.x;
    mMoveForward = moveInput.y;

    mJumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    mAttackPressed = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    mWideAttackPressed = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
    mDodgePressed = glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS;
    mSpecialAttackPressed = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    mRecoverPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
}

void Player::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);

    bool isPlaying = mGame->GetSceneSystem()->IsPlaying();
    if (!isPlaying)
        return;

    if (IsAlive())
        UpdateAlive(deltaTime);
    else
        Die();
}

void Player::UpdateAlive(float deltaTime)
{
    UpdateWorldVec();
    UpdateBoatRide();
    if (mJewelCount < 2 && mJewelTimer <= 0.0f) {
        StartJewelTimer();
    }

    switch (mActionState) {
    case ActionState::Idle:
        UpdateIdle(deltaTime);
        break;

    case ActionState::Dodging:
        UpdateDodging(deltaTime);
        break;

    case ActionState::Attacking:
        UpdateAttacking(deltaTime);
        break;

    case ActionState::Charging:
        UpdateCharging(deltaTime);
        break;

    case ActionState::StrongAttacking:
        UpdateStrongAttacking(deltaTime);
        break;

    case ActionState::KnockedBack:
        UpdateKnockedBack(deltaTime);
        break;
    }

    UpdateTimer(deltaTime);

    mDodgePressedPrev = mDodgePressed;
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
    mRecoverPressedPrev = mRecoverPressed;
}

void Player::Die()
{
    mGame->OnPlayerDied();
}

void Player::Restart()
{
    StartIdle();
    mHp = mMaxHp;
    Respawn();
}

void Player::Respawn()
{
    mPos = mRestartPos;
}

void Player::UpdateWorldVec()
{
    // mForwardVecをmUpVecに垂直な平面へ投影して、地面に沿った前方向を作る
    glm::vec3 projectedForward = mForwardVec - glm::dot(mForwardVec, mUpVec) * mUpVec;

    if (glm::length(projectedForward) < 1e-6f) {
        projectedForward = glm::cross(glm::vec3(1, 0, 0), mUpVec);
        if (glm::length(projectedForward) < 1e-6f)
            projectedForward = glm::cross(glm::vec3(0, 1, 0), mUpVec);
    }

    projectedForward = glm::normalize(projectedForward);
    glm::vec3 baseLeft = glm::normalize(glm::cross(mUpVec, projectedForward));

    // 地面に沿った前方向を、mCameraYaw分だけmUpVec軸まわりに回転させる
    mForwardVec = glm::normalize(projectedForward * std::cos(mCameraYaw) - baseLeft * std::sin(mCameraYaw));
    mLeftVec = glm::normalize(glm::cross(mUpVec, mForwardVec));
}

void Player::UpdateIdle(float deltaTime)
{
    if (!mIsActive)
        return;

    bool canStartCharging = !mOnGround && mAttackPressed && !mIsStrongAttacked;
    if (canStartCharging) {
        StartCharging(deltaTime);
        return;
    }
    if (mAirAttackFloatingTimer <= 0.0f) {
        ApplyGravity(deltaTime);
    }

    bool canStartJumping = mJumpPressed && mOnGround;
    if (canStartJumping && (mSpecialChargingTimer <= 0.0f && !mCanSpecialAttack)) {
        StartJumping(deltaTime);
        return;
    }

    bool canRecover = mRecoverPressed && !mRecoverPressedPrev && mJewelCount > 0 && mHp != mMaxHp;
    if (canRecover) {
        Recover();
    }

    bool isMoving = std::abs(mMoveForward) > 0.01f || std::abs(mMoveLeft) > 0.01f;
    if (isMoving && !mIsTired) {
        ChangeFaceDir();
    }

    if (CanWalk() && (mSpecialChargingTimer <= 0.0f && !mCanSpecialAttack))
        UpdateWalk(deltaTime);

    bool isFalling = glm::dot(mVelocity, mUpVec) < 0.0f;
    if (isFalling)
        mShouldJudgeLanding = true;

    bool canSpecialAttack = mSpecialAttackPressed && mAttackPressed && !mAttackPressedPrev && mJewelCount >= 2;
    if (canSpecialAttack) {
        StartSpecialAttackCharging();
        return;
    }

    if (mWideAttackPressed && !mWideAttackPressedPrev && mIsTired) {
        ReduceTired();
        return;
    }

    bool canContinuousAttacking =
        mSpecialAttackPressed && mWideAttackPressed && !mWideAttackPressedPrev && mJewelCount >= 1;
    if (canContinuousAttacking) {
        StartContinuousAttacking();
        return;
    }

    if (mSpecialChargingTimer >= 0.0f || mCanSpecialAttack) {
        UpdateSpecialAttackCharging(deltaTime);
    }

    if (mContinuousAttackingTimer >= 0.0f) {
        UpdateContinuousAttacking(deltaTime);
        return;
    }

    bool canStartDodging = mDodgeCooldown <= 0.0f && mAttackDodgeLockRemaining <= 0.0f && !mIsDodged && mDodgePressed &&
                           !mDodgePressedPrev;
    if (canStartDodging) {
        StartDodging();
        return;
    }

    bool canStartAttacking = mAttackCooldownRemaining <= 0.0f &&
                             ((mAttackPressed || mWideAttackPressed) && !mAttackPressedPrev && !mWideAttackPressedPrev);
    if (canStartAttacking && (mSpecialChargingTimer <= 0.0f && !mCanSpecialAttack)) {
        StartAttacking(deltaTime);
        return;
    }
}

bool Player::IsFallIntoPlanetInside()
{
    if (mCurrentPlanet->GetPlanetShape() != Planet::PlanetShape::Sphere)
        return false;

    float dist = glm::length(mPos - mCurrentPlanet->GetPos());
    const float planetHalfRadius = mCurrentPlanet->GetRadius() * 0.5f;

    if (dist < planetHalfRadius)
        return true;

    return false;
}

void Player::UpdateDodging(float deltaTime)
{
    MoveDuringDodging(deltaTime);

    mDodgeTimer -= deltaTime;
    if (mDodgeTimer <= 0.0f) {
        StartIdle();
    }
}

void Player::UpdateAttacking(float deltaTime)
{
    if (mOnGround) {
        MoveDuringAttacking(deltaTime);
    }

    if (CanWalk())
        UpdateWalk(deltaTime);

    mAttackMotionTimer -= deltaTime;
    if (mAttackMotionTimer <= 0.0f)
        StartIdle();
}

void Player::UpdateCharging(float deltaTime)
{
    bool isAttackBtnReleased = !mAttackPressed;
    if (isAttackBtnReleased) {
        StartStrongAttacking(deltaTime);
        return;
    }

    if (mAttackPressTimer < 0.0f)
        return;

    mAttackPressTimer -= deltaTime;
    if (mAttackPressTimer >= 0.0f) {
        MoveDuringCharging(deltaTime);
        return;
    }

    FinishCharging();
}

void Player::UpdateStrongAttacking(float deltaTime)
{
    MoveDuringStrongAttacking(deltaTime);

    mStrongAttackTimer -= deltaTime;
    if (mStrongAttackTimer >= 0.0f)
        return;

    StartIdle();

    if (!mIsCharged)
        return;

    if (!mIsStrongAttackHit) {
        Attack(deltaTime);
    }

    if (mIsStrongAttackHit) {
        mIsStrongAttackHit = false;
        mGame->OnStrongAttacked();
    }
}

void Player::UpdateKnockedBack(float deltaTime)
{
    MoveDuringKnockBack(deltaTime);

    mDamageTimer -= deltaTime;
    if (mDamageTimer <= 0.0f)
        StartIdle();
}

void Player::UpdateSpecialAttackCharging(float deltaTime)
{
    float specialChargingTimerPrev = mSpecialChargingTimer;
    mSpecialChargingTimer -= deltaTime;
    if (specialChargingTimerPrev >= 2.0f && mSpecialChargingTimer <= 2.0f) {
        mGame->VibrateController(10000, 0, 1000);
        mJewelCount--;
        mGame->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 1.0f && mSpecialChargingTimer <= 1.0f) {
        mGame->VibrateController(20000, 0, 1000);
        mJewelCount--;
        mGame->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 0.0f && mSpecialChargingTimer <= 0.0f) {
        mGame->VibrateController(30000, 0, 1000);
        mGame->GetAudioSystem()->PlaySE("charged_se");
    }

    if (mSpecialChargingTimer <= 0.0f) {
        mCanSpecialAttack = true;
    }

    if (mSpecialChargingTimer <= 0.0f && mSpecialAttackPressedPrev && !mSpecialAttackPressed) {
        SpecialAttack(deltaTime);
    }

    if (mSpecialAttackPressedPrev && !mSpecialAttackPressed) {
        mSpecialChargingTimer = -1.0f;
    }
}

void Player::UpdateTimer(float deltaTime)
{
    if (mAirAttackFloatingTimer > 0.0f) {
        mAirAttackFloatingTimer -= deltaTime;
    }

    if (mDodgeCooldown > 0.0f)
        mDodgeCooldown -= deltaTime;

    if (mJewelTimer >= 0.0f) {
        UpdateJewelTimer(deltaTime);
    }

    if (mAttackCooldownRemaining >= 0.0f)
        mAttackCooldownRemaining -= deltaTime;

    if (mAttackMoveLockRemaining > 0.0f) {
        mAttackMoveLockRemaining -= deltaTime;
        if (mIsTired && mAttackMoveLockRemaining <= 0.0f) {
            mIsTired = false;
        }
    }

    if (mAttackDodgeLockRemaining > 0.0f)
        mAttackDodgeLockRemaining -= deltaTime;

    if (mInvincibleTimer >= 0.0f)
        mInvincibleTimer -= deltaTime;

    if (mRayCastTimer >= 0.0f)
        mRayCastTimer -= deltaTime;

    if (mInputAvailableTimer >= 0.0f) {
        mInputAvailableTimer -= deltaTime;
    }

    if (mComboKeepTimer > 0.0f) {
        UpdateComboKeepTimer(deltaTime);
    }
}

void Player::UpdateJewelTimer(float deltaTime)
{
    mJewelTimer -= deltaTime;
    if (mJewelTimer >= 0.0f)
        return;

    if (mJewelCount < 2) {
        mJewelCount++;
    }
}

void Player::UpdateComboKeepTimer(float deltaTime)
{
    mComboKeepTimer -= deltaTime;
    if (mComboKeepTimer >= 0.0f)
        return;

    mAttackComboIndex = 0;
}

void Player::UpdateWalk(float deltaTime)
{
    glm::vec3 moveDelta =
        mForwardVec * mMoveForward * mMoveSpeed * deltaTime + mLeftVec * mMoveLeft * mMoveSpeed * deltaTime;
    glm::vec3 desiredPos = mPos + moveDelta;

    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);
    mPos = desiredPos;
}

void Player::StartJewelTimer()
{
    mJewelTimer = 30.0f;
}

void Player::UpdateBoatRide()
{
    const std::vector<Boat*>& boats = mCurrentPlanet->GetBoats();
    if (boats.empty())
        return;

    for (auto boat : boats) {
        if (!boat->GetIsActive())
            continue;

        if (boat->GetIsMoving()) {
            FollowMovingBoat(boat);
            return;
        }

        if (IsTouchingBoat(boat)) {
            StartRidingBoat(boat);
            return;
        }
    }
}

void Player::StartIdle()
{
    mActionState = ActionState::Idle;
}

void Player::StartDodging()
{
    mActionState = ActionState::Dodging;

    if (mMoveForward != 0.0f || mMoveLeft != 0.0f)
        mDodgeDir = mFacingForwardVec;
    else
        mDodgeDir = -mFacingForwardVec;

    mDodgeTimer = (mOnGround) ? mDodgeDuration : mDodgeDuration * 4.0f;
    mDodgeCooldown = mDodgeCooldownTime;
    mInvincibleTimer = mDodgeDuration;

    mVelocity = glm::vec3(0.0f);
    mGame->GetAudioSystem()->PlaySE("dodge_se");
    mIsDodged = true;
}

void Player::StartAttacking(float deltaTime)
{
    mActionState = ActionState::Attacking;

    if (!mOnGround && mWideAttackPressed) {
        mAttackKind = AttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack / 2.0f;
        mAirAttackFloatingTimer = 0.5f;
        mIsAirAttacking = true;
        Attack(deltaTime);
        return;
    }

    if (!mOnGround) {
        return;
    }

    if (mAttackPressed) {
        mAttackKind = AttackKind::Normal;
        mAttackRange = mNormalAttackRange;
        mAttackAngle = mNormalAttackAngle;
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttack = mNormalAttack;
    } else if (mWideAttackPressed) {
        mAttackKind = AttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack;
    }

    Attack(deltaTime);
}

void Player::StartCharging(float deltaTime)
{
    mActionState = ActionState::Charging;
    mAttackPressTimer = mDefaultAttackPressTimer;
    mGame->GetAudioSystem()->PlaySE("air_charging_se");
}

void Player::StartStrongAttacking(float deltaTime)
{
    mActionState = ActionState::StrongAttacking;
    mAttackKind = AttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;

    float pressTime = std::min(1.0f, mDefaultAttackPressTimer - mAttackPressTimer / mDefaultAttackPressTimer);
    mStrongAttackTimer = mDefaultStrongAttackTimer * pressTime;
    mIsStrongAttacked = true;
}

void Player::StartJumping(float deltaTime)
{
    constexpr float jumpPower = 6.0f;
    mVelocity += mUpVec * jumpPower;
    mPos += mVelocity * deltaTime;
    mOnGround = false;
    mShouldJudgeLanding = false;
    mGame->GetAudioSystem()->PlaySE("jump_se");
}

void Player::FinishCharging()
{
    mGame->OnPlayerFinishCharging();
    mIsCharged = true;
}

void Player::MoveDuringDodging(float deltaTime)
{
    float dodgeSpeed = (mOnGround) ? (mDodgeDistance / mDodgeDuration) : (mDodgeDistance / (mDodgeDuration * 4.0f));

    if (mSpecialChargingTimer >= 0.0f || mCanSpecialAttack) {
        dodgeSpeed /= 4.0f;
    }
    const glm::vec3 moveDelta = mDodgeDir * dodgeSpeed * deltaTime;
    glm::vec3 desiredPos = mPos + moveDelta;

    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);
    mPos = desiredPos;

    if (mOnGround) {
        SnapToGround(0.5f, 1.0f);
    }
}

void Player::SnapToGround(float upOffset, float downLength)
{
    if (glm::length(mUpVec) < 1e-6f) {
        return;
    }

    glm::vec3 up = glm::normalize(mUpVec);

    glm::vec3 from = mPos + up * upOffset;
    glm::vec3 to = mPos - up * downLength;

    btCollisionWorld::ClosestRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

    auto* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return;
    }

    bulletWorld->rayTest(cb.m_rayFromWorld, cb.m_rayToWorld, cb);

    if (!cb.hasHit()) {
        return;
    }

    glm::vec3 hitPos(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());

    mPos = hitPos;
    mOnGround = true;
    mVelocity = glm::vec3(0.0f);
}

void Player::MoveDuringCharging(float deltaTime)
{
    const glm::vec3 moveDelta = -mFacingForwardVec * mChargeMoveSpeed * deltaTime;
    glm::vec3 desiredPos = mPos + moveDelta;
    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);
    mPos = desiredPos;

    UpdateFacingForwardVec();
}

void Player::MoveDuringStrongAttacking(float deltaTime)
{
    const glm::vec3 moveDelta = mFacingForwardVec * mStrongAttackSpeed * deltaTime;
    glm::vec3 desiredPos = mPos + moveDelta;
    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);
    mPos = desiredPos;

    UpdateFacingForwardVec();
}

void Player::MoveDuringAttacking(float deltaTime)
{
    glm::vec3 moveDelta = mFacingForwardVec * mAttackSpeed * deltaTime;
    glm::vec3 desiredPos = mPos + moveDelta;

    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);
    mPos = desiredPos;

    UpdateFacingForwardVec();
}

void Player::MoveDuringKnockBack(float deltaTime)
{
    glm::vec3 toPlayer = glm::normalize(mPos - mKnockBackFrom);
    mPos += toPlayer * mKnockBackSpeed * deltaTime;
}

void Player::ChangeFaceDir()
{
    glm::vec3 moveDir = glm::normalize(mForwardVec * mMoveForward + mLeftVec * mMoveLeft);
    mFacingForwardVec = moveDir;
    mFacingYaw = mGame->GetMathUtils()->GetYawFromDirection(mUpVec, moveDir) + 3.14159265f;
}

void Player::UpdateFacingForwardVec()
{
    glm::vec3 up = mUpVec;
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = mFacingForwardVec;
    forward = forward - up * glm::dot(forward, up);

    if (glm::length(forward) < 1e-6f) {
        forward = mForwardVec;
        forward = forward - up * glm::dot(forward, up);
    }

    if (glm::length(forward) < 1e-6f) {
        return;
    }

    mFacingForwardVec = glm::normalize(forward);
    mFacingYaw = mGame->GetMathUtils()->GetYawFromDirection(up, mFacingForwardVec) + 3.14159265f;
}

void Player::Attack(float deltaTime)
{
    std::vector<Enemy*> hitEnemies = FindHitEnemies();
    if (hitEnemies.empty()) {
        StartAfterAttackReaction();
        mGame->GetAudioSystem()->PlaySE("attack_miss_se");

        if (mAttackComboIndex != 3)
            return;

        mAttackComboIndex = 0;
        return;
    }

    if (mAttackKind != AttackKind::Strong) {
        mGame->OnPlayerAttackHit();
        StartAfterAttackReaction();
        if (mOnGround) {
            for (Enemy* enemy : hitEnemies) {
                enemy->ApplyDamage(mAttack, this);
            }
        } else {
            bool isHit = false;
            for (Enemy* enemy : hitEnemies) {
                if (enemy->GetOnGround()) {
                    continue;
                }
                enemy->ApplyDamage(mAttack, this);
                isHit = true;
            }
            if (isHit) {
                mGame->GetAudioSystem()->PlaySE("attack_se");
            } else {
                mGame->GetAudioSystem()->PlaySE("attack_miss_se");
            }
            return;
        }

        if (mAttackComboIndex != 3) {
            mGame->GetAudioSystem()->PlaySE("attack_se");
            return;
        }

        mAttackComboIndex = 0;
        mGame->GetAudioSystem()->PlaySE("destroy_se");
        for (Enemy* enemy : hitEnemies) {
            if (enemy->GetOnGround()) {
                enemy->ApplyBreak(deltaTime);
            }
        }
        return;
    }

    GetGame()->GetAudioSystem()->PlaySE("attack_air_se");
    StartTired(5.0f);
    for (Enemy* enemy : hitEnemies) {
        enemy->SetIsStrongAttacked(true);
        enemy->ApplyDamage(mAttack, this);
        mIsStrongAttackHit = true;
    }
}

void Player::StartTired(float lockTime)
{
    mIsTired = true;
    mAttackMoveLockRemaining = lockTime;
    mDodgeCooldown = lockTime;
    mAttackCooldownRemaining = lockTime;
}

void Player::StartAfterAttackReaction()
{
    mAttackMoveLockRemaining = 0.2f;
    mComboKeepTimer = mAttackMoveLockRemaining + 1.0f;

    if (mOnGround)
        mAttackMotionTimer = mDefaultAttackMotionTimer;

    mAttackComboIndex++;

    if (mAttackKind == AttackKind::Normal && mAttackComboIndex != 3) {
        mAttackComboIndex = 0;
        return;
    }

    if (mAttackKind == AttackKind::Strong) {
        StartTired(5.0f);
        return;
    }

    if (mAttackComboIndex != 3) {
        return;
    }

    if (mAttackKind == AttackKind::Normal) {
        mAttackMoveLockRemaining = 1.0f;
    }

    if (mAttackKind == AttackKind::Wide && mOnGround) {
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttackMoveLockRemaining = 0.8f;
    }
}

std::vector<Enemy*> Player::FindHitEnemies()
{
    std::vector<Enemy*> hitEnemies;

    for (Enemy* enemy : mCurrentPlanet->GetEnemies()) {
        if (enemy->GetIsDead())
            continue;

        if (mAttackKind == AttackKind::Strong && enemy->GetOnGround())
            continue;

        const glm::vec3 enemyPos = enemy->GetPos();
        const glm::vec3 toEnemy =
            glm::normalize((enemyPos + enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f)) - mPos);
        const float dist = glm::length(enemyPos - mPos);
        const float dot = glm::dot(mFacingForwardVec, toEnemy);
        const float effectiveRange = mAttackRange + enemy->GetRadius();

        if (IsEnemyHitByAttack(dist, dot, effectiveRange))
            hitEnemies.push_back(enemy);
    }

    return hitEnemies;
}

bool Player::IsEnemyHitByAttack(float dist, float dot, float effectiveRange)
{
    float threshold = std::cos(mAttackAngle * 0.5f);

    return dist <= effectiveRange && dot >= threshold;
}

void Player::StartSpecialAttackCharging()
{
    mSpecialChargingTimer = 3.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle / 2.0f;
}

void Player::StartContinuousAttacking()
{
    mJewelCount--;
    mContinuousAttackingTimer = 6.0f;
}

void Player::UpdateContinuousAttacking(float deltaTime)
{
    mAttackKind = AttackKind::Wide;
    mAttack = mWideAttack / 2.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;
    mContinuousAttackingTimer -= deltaTime;
    mContinuousAttackingCooldown -= deltaTime;
    if (mContinuousAttackingCooldown <= 0.0f) {
        mContinuousAttackingCooldown = 0.25f;
        Attack(deltaTime);
        mAttackMoveLockRemaining = 0.0f;
    }
}

void Player::SpecialAttack(float deltaTime)
{
    std::vector<Enemy*> enemies = FindHitEnemies();
    for (auto& enemy : enemies) {
        if (enemy->GetIsDead())
            continue;

        if (enemy->GetOnGround()) {
            while (enemy->GetBreakCount()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        if (enemy->GetCanCountered()) {
            enemy->ApplyDamage(600, this);
            enemy->FlipCanCountered();
            mJewelCount = 2;
            mGame->GetAudioSystem()->PlaySE("just_attack_se");
        } else {
            enemy->ApplyDamage(300, this);
        }
    }

    mGame->VibrateController(0, 40000, 1000);
    mCanSpecialAttack = false;
}

void Player::Recover()
{
    mJewelCount--;
    mHp += 1;
    mGame->GetAudioSystem()->PlaySE("recover_se");

    if (mHp >= mMaxHp) {
        mHp = mMaxHp;
    }
}

void Player::ApplyDamage(Enemy* enemy, float deltaTime)
{
    if (mActionState == ActionState::Dodging && enemy->GetCanCountered()) {
        mGame->OnPlayerCounter();
        enemy->ApplyBreak(deltaTime, true);
        enemy->FlipCanCountered();
        mGame->GetAudioSystem()->PlaySE("just_dodge_se");
        if (mJewelCount < 2) {
            mJewelCount++;
        }
        return;
    }

    if (mInvincibleTimer >= 0.0f) {
        return;
    }

    if (mCanSpecialAttack) {
        StartTired(20.0f);
    }
    mHp -= enemy->GetAttack();
    mKnockBackFrom = enemy->GetPos();
    mDamageTimer = mDefaultDamageTimer;
    mInvincibleTimer = mDefaultInvincibleTimer;
    mActionState = ActionState::KnockedBack;
    mGame->OnPlayerApplyDamage();
    mCanSpecialAttack = false;
    mSpecialChargingTimer = -1.0f;
    mContinuousAttackingTimer = -1.0f;
}

void Player::FollowMovingBoat(Boat* boat)
{
    mPos = boat->GetPos();
}

bool Player::IsTouchingBoat(Boat* boat)
{
    float distToBoat = glm::length(mPos - boat->GetPos());
    constexpr float boatTouchRadius = 0.9f;
    return distToBoat <= boatTouchRadius;
}

void Player::StartRidingBoat(Boat* boat)
{
    if (!mIsActive)
        return;

    boat->StartTravel();
    mIsActive = false;
}

void Player::OnBoatArrived(Boat* boat)
{
    mCurrentPlanetNum++;

    mPos = boat->GetDestPos();
    mRestartPos = mPos;
    mRestartPlanetIndex = mCurrentPlanetNum;

    mVelocity = glm::vec3(0.0f);
    mIsActive = true;

    UpdateFallbackUpVec();
}

void Player::OnLanded()
{
    mIsDodged = false;
    mIsStrongAttacked = false;
    mIsCharged = false;
    mIsAirAttacking = false;
    mGame->OnLanded();
}

void Player::OnUpVecUpdateFailed()
{
    if (mRayCastTimer > 0.0f) {
        return;
    }

    UpdateFallbackUpVec();
    SetVelocity(glm::vec3(0.0f));
    SetRayCastTimer(0.5f);
}

void Player::OnCastSucceeded()
{
    mRayCastTimer = 0.5f;
}

void Player::ReduceTired()
{
    mAttackMoveLockRemaining -= 0.8f;
    mDodgeCooldown -= 0.8f;
    mAttackCooldownRemaining -= 0.8f;

    if (mAttackMoveLockRemaining <= 0.0f) {
        mIsTired = false;
    }
}