#include "Enemy.h"

#include "CharacterActor.h"
#include "Game.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyConfig.h"
#include "actor/enemy/EnemyConfigLoader.h"
#include "actor/enemy/EnemyDamageHandler.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/behavior/EnemyBehaviorController.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <limits>

namespace {
constexpr float dormantEnemyUpdateIntervalSeconds = 0.25f;
constexpr float minimumFullRateDistance = 30.0f;
constexpr float fullRateDistanceMargin = 5.0f;
constexpr float groundedPositionEpsilon = 0.0001f;

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
} // namespace

Enemy::Enemy(Game* game)
    : CharacterActor(game),
      mStateMachine(std::make_unique<EnemyStateMachine>()),
      mMovement(std::make_unique<EnemyMovement>()),
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
    Game* game = GetGame();
    Planet* planet = GetCurrentPlanet();
    if (!game ||
        !planet ||
        !mStateMachine->IsAlive() ||
        mStateMachine->GetActionState() != ActionState::Idle ||
        !mOnGround ||
        !mHasRecordedGroundedTransform ||
        GetGroundActor() != planet ||
        GetIsEditorSelected()) {
        return false;
    }

    const float fullRateDistance =
        std::max(
            minimumFullRateDistance,
            GetDetectionRange() + fullRateDistanceMargin);
    const float fullRateDistanceSquared =
        fullRateDistance * fullRateDistance;

    for (Player* player : game->GetPlayers()) {
        if (!player ||
            !player->GetIsActive() ||
            !player->IsAlive() ||
            player->GetCurrentPlanet() != planet) {
            continue;
        }

        const glm::vec3 enemyToPlayer =
            player->GetPos() - GetPos();
        const float distanceSquared =
            glm::dot(enemyToPlayer, enemyToPlayer);
        if (distanceSquared <= fullRateDistanceSquared) {
            return false;
        }
    }

    return true;
}

bool Enemy::ShouldUpdateUpVecEveryFrame() const
{
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

void Enemy::ApplyConfig(const std::string& type)
{
    const EnemyConfig config = EnemyConfigLoader::Load("../assets/data/actor/enemies.yaml", type);
    ApplyEnemyConfig(config);
}

void Enemy::ApplyEnemyConfig(const EnemyConfig& config)
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

void Enemy::ApplyDamage(float damage, Player* player)
{
    mDamageHandler->ApplyDamage(*this, mStatus, *mStateMachine, damage, player);
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
    const glm::vec3& dodgeDirection)
{
    if (GetIsBoss() ||
        IsOnGround() ||
        !IsAlive()) {
        return;
    }

    mMovement->ApplyAirDodgePush(
        *this,
        dodgeDirection);
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
