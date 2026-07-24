#include "Enemy.h"

#include "CharacterActor.h"
#include "Game.h"
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

void Enemy::ApplyConfig(const std::string& type)
{
    const EnemyConfig config = EnemyConfigLoader::Load("../assets/data/actor/enemies.yaml", type);
    ApplyEnemyConfig(config);
}

void Enemy::ApplyEnemyConfig(const EnemyConfig& config)
{
    SetIsBoss(config.isBoss);
    SetKnockBackSpeed(config.knockBackSpeed);
    SetDefaultLaunchedTimer(config.defaultLaunchedTimer);
    SetDetectionRange(config.detectionRange);

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

    mBehaviorController->Configure(config.behavior);
}

void Enemy::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);

    if (!GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    mStatus.SetNearestPlayer(GetGame()->FindNearestPlayer(this));

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

void Enemy::ApplyDamage(float damage, Player* player)
{
    mDamageHandler->ApplyDamage(*this, mStatus, *mStateMachine, damage, player);
}

void Enemy::ApplyBreak(float deltaTime, bool isAllBreak)
{
    mCombat->ApplyBreak(*this, mStatus, *mMovement, *mStateMachine, deltaTime, isAllBreak);
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
