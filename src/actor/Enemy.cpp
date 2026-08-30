#include "Enemy.h"

#include "CharacterActor.h"
#include "Game.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyConfig.h"
#include "actor/enemy/EnemyDamageHandler.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/behavior/EnemyBehaviorController.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>

namespace {
constexpr float dormantEnemyUpdateIntervalSeconds = 0.25f;
constexpr float groundedPositionEpsilon = 0.0001f;
constexpr float normalEnemySpinDurationSeconds = 0.42f;
constexpr float bossSquashDurationSeconds = 0.10f;
constexpr float bossStretchDurationSeconds = 0.12f;
constexpr float bossRecoveryDurationSeconds = 0.16f;
constexpr float bossHitReactionDurationSeconds =
    bossSquashDurationSeconds +
    bossStretchDurationSeconds +
    bossRecoveryDurationSeconds;

float CalculateSmoothstep(float progress)
{
    const float clampedProgress = glm::clamp(progress, 0.0f, 1.0f);
    return clampedProgress * clampedProgress *
           (3.0f - 2.0f * clampedProgress);
}

Player* FindNearestPlayerOnSameSurfaceFace(const Enemy& enemy)
{
    Game* game = enemy.GetGame();
    Planet* planet = enemy.GetCurrentPlanet();
    if (!game || !planet) {
        return nullptr;
    }

    Player* nearestPlayer = nullptr;
    float nearestDistanceSquared =
        std::numeric_limits<float>::max();
    for (Player* player : game->GetPlayers()) {
        if (!player ||
            !player->GetIsActive() ||
            !player->IsAlive() ||
            player->GetCurrentPlanet() != planet ||
            !planet->ArePositionsOnSameSurfaceFace(
                enemy.GetPos(),
                player->GetPos())) {
            continue;
        }

        const glm::vec3 enemyToPlayer =
            player->GetPos() - enemy.GetPos();
        const float distanceSquared =
            glm::dot(enemyToPlayer, enemyToPlayer);
        if (distanceSquared >= nearestDistanceSquared) {
            continue;
        }

        nearestDistanceSquared = distanceSquared;
        nearestPlayer = player;
    }
    return nearestPlayer;
}
}

Enemy::Enemy(Game* game)
    : CharacterActor(game),
      mStateMachine(std::make_unique<EnemyStateMachine>()),
      mMovement(std::make_unique<EnemyMovement>(
          *game->GetPhysicsSystem(),
          *game->GetMathUtils())),
      mCombat(std::make_unique<EnemyCombat>()),
      mDamageHandler(std::make_unique<EnemyDamageHandler>()),
      mBehaviorController(std::make_unique<EnemyBehaviorController>())
{
}

Enemy::~Enemy() = default;

float Enemy::ResolveMinimumUpdateIntervalSeconds() const
{
    return CanUseReducedUpdateRate()
        ? dormantEnemyUpdateIntervalSeconds
        : 0.0f;
}

bool Enemy::CanUseReducedUpdateRate() const
{
    Planet* planet = GetCurrentPlanet();
    if (!planet ||
        mShouldUseFullRateUpdate ||
        !mStateMachine->IsAlive() ||
        mStateMachine->GetActionState() != ActionState::Idle ||
        mHitReactionKind != HitReactionKind::None ||
        !mOnGround ||
        !mHasRecordedGroundedTransform ||
        GetGroundActor() != planet ||
        GetIsEditorSelected()) {
        return false;
    }
    return true;
}

bool Enemy::ShouldUpdateUpVecEveryFrame() const
{
    if (GetIsBoss() && GetLifeState() == LifeState::Dying) {
        return false;
    }

    if (!mHasRecordedGroundedTransform ||
        !mOnGround ||
        GetGroundActor() != GetCurrentPlanet()) {
        return true;
    }

    const glm::vec3 positionChange =
        GetPos() - mLastGroundedPosition;
    return glm::dot(positionChange, positionChange) >
        groundedPositionEpsilon * groundedPositionEpsilon;
}

bool Enemy::ShouldAcceptLandingSurface(
    Actor* surfaceActor,
    const glm::vec3& surfaceNormal) const
{
    (void)surfaceNormal;

    return dynamic_cast<Enemy*>(surfaceActor) == nullptr;
}

void Enemy::ApplyConfig(const EnemyConfig& config)
{
    SetIsBoss(config.isBoss);
    SetIsNormalHitKnockBackEnabled(
        config.isNormalHitKnockBackEnabled);
    SetKnockBackSpeed(config.knockBackSpeed);
    SetDefaultLaunchedTimer(config.defaultLaunchedTimer);
    SetLaunchHeight(config.launchHeight);
    SetDetectionRange(config.detectionRange);
    SetAttackPreparationRange(config.attackPreparationRange);

    SetHp(config.hp);
    SetMaxHp(config.hp);
    SetScale(glm::vec3(config.scale));
    SetMoveSpeed(config.moveSpeed);
    SetAttack(config.attack);
    SetRadius(config.radius);

    SetBreakCountMax(config.breakCountMax);
    SetBreakCount(config.breakCountMax);

    SetModelPath(config.modelPath);

    SetDefaultStandByAttackTimer(config.defaultStandByAttackTimer);
    SetDefaultAttackMotionTimer(config.defaultAttackMotionTimer);
    SetAttackSpeed(config.attackSpeed);

    mStateMachine->ConfigureBossManeuver(config.bossManeuver);
    mBehaviorController->Configure(config.behavior);
}

void Enemy::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);
    UpdateHitReaction(deltaTime);

    if (mStateMachine->IsAlive() &&
        (mOnGround || !mHasRecordedGroundedTransform)) {
        mLastGroundedPosition = GetPos();
        if (glm::length(GetUpVec()) > 0.000001f) {
            mLastGroundedUpDirection =
                glm::normalize(GetUpVec());
        }
        mHasRecordedGroundedTransform = true;
    }

    if (!GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    mMovement->SeparateAfterOverlappingEnemyLanding(
        *this,
        deltaTime);

    mStatus.SetNearestPlayer(
        FindNearestPlayerOnSameSurfaceFace(*this));

    switch (mStateMachine->GetLifeState()) {
    case LifeState::Alive:
        mStateMachine->UpdateAlive(
            *this, mStatus, *mMovement, *mCombat, *mBehaviorController, deltaTime);
        break;

    case LifeState::Dying:
        mStateMachine->UpdateDying(*this, mStatus, *mMovement, deltaTime);
        break;

    case LifeState::Dead:
        break;
    }
}

bool Enemy::ShouldRenderSolidWhite() const
{
    if (!IsAlive()) {
        return false;
    }

    constexpr float recoveryWarningDurationSeconds = 1.5f;
    const float launchedTimerSeconds = mStatus.GetLaunchedTimer();
    const bool shouldBlink =
        mStateMachine->GetActionState() == ActionState::Launched &&
        launchedTimerSeconds >= 0.0f &&
        launchedTimerSeconds <= recoveryWarningDurationSeconds;
    if (!shouldBlink) {
        return false;
    }

    constexpr float blinkIntervalSeconds = 0.12f;
    const float warningElapsedSeconds =
        recoveryWarningDurationSeconds - launchedTimerSeconds;
    const int blinkPhase =
        static_cast<int>(warningElapsedSeconds / blinkIntervalSeconds);

    return (blinkPhase % 2) != 0;
}

glm::quat Enemy::GetRenderModelRotationOffset() const
{
    if (mHitReactionKind != HitReactionKind::NormalEnemySpin) {
        return Actor::GetRenderModelRotationOffset();
    }

    const float progress = glm::clamp(
        mHitReactionElapsedSeconds /
            normalEnemySpinDurationSeconds,
        0.0f,
        1.0f);
    const float angleRadians =
        glm::two_pi<float>() * CalculateSmoothstep(progress);
    return glm::angleAxis(
        angleRadians,
        glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Enemy::GetRenderScale() const
{
    const glm::vec3 baseScale = GetScale();
    if (mHitReactionKind != HitReactionKind::BossSquashStretch) {
        return baseScale;
    }

    const glm::vec3 normalScaleMultiplier(1.0f);
    const glm::vec3 squashScaleMultiplier(1.18f, 0.72f, 1.18f);
    const glm::vec3 stretchScaleMultiplier(0.92f, 1.12f, 0.92f);

    glm::vec3 reactionScaleMultiplier(1.0f);
    if (mHitReactionElapsedSeconds < bossSquashDurationSeconds) {
        const float progress = CalculateSmoothstep(
            mHitReactionElapsedSeconds /
            bossSquashDurationSeconds);
        reactionScaleMultiplier = glm::mix(
            normalScaleMultiplier,
            squashScaleMultiplier,
            progress);
    } else if (mHitReactionElapsedSeconds <
               bossSquashDurationSeconds + bossStretchDurationSeconds) {
        const float stretchElapsedSeconds =
            mHitReactionElapsedSeconds - bossSquashDurationSeconds;
        const float progress = CalculateSmoothstep(
            stretchElapsedSeconds /
            bossStretchDurationSeconds);
        reactionScaleMultiplier = glm::mix(
            squashScaleMultiplier,
            stretchScaleMultiplier,
            progress);
    } else {
        const float recoveryElapsedSeconds =
            mHitReactionElapsedSeconds -
            bossSquashDurationSeconds -
            bossStretchDurationSeconds;
        const float progress = CalculateSmoothstep(
            recoveryElapsedSeconds /
            bossRecoveryDurationSeconds);
        reactionScaleMultiplier = glm::mix(
            stretchScaleMultiplier,
            normalScaleMultiplier,
            progress);
    }

    return baseScale * reactionScaleMultiplier;
}

void Enemy::StartNormalHitReaction()
{
    mHitReactionKind = HitReactionKind::NormalEnemySpin;
    mHitReactionElapsedSeconds = 0.0f;
}

void Enemy::StartBossHitReaction()
{
    mHitReactionKind = HitReactionKind::BossSquashStretch;
    mHitReactionElapsedSeconds = 0.0f;
}

void Enemy::UpdateHitReaction(float deltaTime)
{
    if (mHitReactionKind == HitReactionKind::None) {
        return;
    }

    mHitReactionElapsedSeconds += std::max(0.0f, deltaTime);
    const float durationSeconds =
        mHitReactionKind == HitReactionKind::NormalEnemySpin
            ? normalEnemySpinDurationSeconds
            : bossHitReactionDurationSeconds;
    if (mHitReactionElapsedSeconds < durationSeconds) {
        return;
    }

    mHitReactionKind = HitReactionKind::None;
    mHitReactionElapsedSeconds = 0.0f;
}

void Enemy::ApplyDamage(float damage, Player* player)
{
    mDamageHandler->ApplyDamage(
        *this,
        mStatus,
        *mStateMachine,
        *mMovement,
        damage,
        player);
}

void Enemy::DefeatImmediately()
{
    if (GetIsDead()) {
        return;
    }

    mStatus.SetHpZero();
    mStateMachine->FinishDying(*this, mStatus);
}

void Enemy::ApplyBreak(float deltaTime, bool isAllBreak)
{
    mCombat->ApplyBreak(*this, mStatus, *mMovement, *mStateMachine, deltaTime, isAllBreak);
}

void Enemy::ApplyAirDodgePush(
    const glm::vec3& dodgeDirection,
    float pushSpeed,
    float pushDampingPerSecond)
{
    if (GetIsBoss() ||
        IsOnGround() ||
        !IsAlive()) {
        return;
    }

    mMovement->ApplyAirDodgePush(
        *this,
        dodgeDirection,
        pushSpeed,
        pushDampingPerSecond);
}

const char* Enemy::GetCurrentBehaviorActionType() const
{
    return mBehaviorController->GetCurrentActionType();
}

const std::string& Enemy::GetBehaviorProfileName() const
{
    return mBehaviorController->GetProfileName();
}

bool Enemy::GetBehaviorAttackPreview(EnemyAttackPreview& preview) const
{
    return mBehaviorController->GetCurrentAttackPreview(preview);
}
