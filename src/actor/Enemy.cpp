#include "Enemy.h"
#include "CharacterActor.h"
#include "Game.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include <yaml-cpp/yaml.h>

Enemy::Enemy(Game* game)
    : CharacterActor(game),
      mLifeState(LifeState::Alive),
      mActionState(ActionState::Idle),
      mStateMachine(std::make_unique<EnemyStateMachine>()),
      mMovement(std::make_unique<EnemyMovement>()),
      mCombat(std::make_unique<EnemyCombat>())
{
}

Enemy::~Enemy() = default;

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}
} // namespace

void Enemy::ApplyConfig(const std::string& type)
{
    SetIsBoss(type == "boss");

    YAML::Node enemyRoot = YAML::LoadFile("../assets/data/actor/enemies.yaml");

    if (!enemyRoot["enemies"] || !enemyRoot["enemies"].IsSequence()) {
        return;
    }

    for (const YAML::Node& enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = ReadString(enemyNode, "type", "");

        if (enemyType == "common") {
            SetKnockBackSpeed(ReadFloat(enemyNode, "knockBackSpeed", 0.0f));
            SetDefaultLaunchedTimer(ReadFloat(enemyNode, "defaultLaunchedTimer", 0.0f));
            SetDetectionRange(ReadFloat(enemyNode, "detectionRange", 0.0f));
            continue;
        }

        if (type != enemyType) {
            continue;
        }

        const float hp = ReadFloat(enemyNode, "hp", 80.0f);
        SetHp(hp);
        SetMaxHp(hp);

        const float scale = ReadFloat(enemyNode, "scale", 0.25f);
        SetScale(glm::vec3(scale));

        SetMoveSpeed(ReadFloat(enemyNode, "speed", 1.0f));
        SetAttack(ReadFloat(enemyNode, "attack", 5.0f));
        SetRadius(ReadFloat(enemyNode, "radius", 0.75f));

        const int breakCountMax = ReadInt(enemyNode, "breakCountMax", 1);
        SetBreakCountMax(breakCountMax);
        SetBreakCount(breakCountMax);

        SetModelPath(ReadString(enemyNode, "modelPath", ""));

        SetDefaultStandByAttackTimer(ReadFloat(enemyNode, "defaultStandByAttackTimer", 0.0f));
        SetDefaultAttackMotionTimer(ReadFloat(enemyNode, "defaultAttackMotionTimer", 0.0f));
        SetAttackSpeed(ReadFloat(enemyNode, "attackSpeed", 0.0f));
    }
}

void Enemy::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);

    if (!GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    mStatus.SetNearestPlayer(GetGame()->FindNearestPlayer(this));

    switch (mLifeState) {
    case LifeState::Alive:
        mStateMachine->UpdateAlive(*this, mStatus, *mMovement, *mCombat, deltaTime);
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
    mCombat->ApplyDamage(*this, mStatus, *mStateMachine, damage, player);
}

void Enemy::ApplyBreak(float deltaTime, bool isAllBreak)
{
    mCombat->ApplyBreak(*this, mStatus, *mMovement, *mStateMachine, deltaTime, isAllBreak);
}
