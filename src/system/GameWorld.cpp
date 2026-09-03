#include "system/GameWorld.h"

#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/Player.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <glm/glm.hpp>

namespace {
constexpr std::size_t fullRateEnemyCount = 24;
constexpr float enemyUpdatePriorityRefreshIntervalSeconds = 0.25f;

struct EnemyDistance {
    Enemy* enemy = nullptr;
    float nearestPlayerDistanceSquared = 0.0f;
};
}

void GameWorld::CreateStages(int stageCount)
{
    mStages.clear();
    mStagesUnique.clear();
    mCurrentStage = nullptr;
    mCurrentStageNum = 0;
    mEnemyUpdatePriorityRefreshRemainingSeconds = 0.0f;

    for (int i = 0; i < stageCount; i++) {
        auto stageUnique = std::make_unique<Stage>();
        Stage* stage = stageUnique.get();

        mStagesUnique.emplace_back(std::move(stageUnique));
        mStages.emplace_back(stage);

        if (i == 0) {
            mCurrentStage = stage;
        }
    }
}

void GameWorld::AddActor(std::unique_ptr<Actor> actor)
{
    mActors.emplace_back(std::move(actor));
}

void GameWorld::AddPlayer(Player* player)
{
    mPlayers.emplace_back(player);
}

void GameWorld::RemoveAllPlayers()
{
    mPlayers.clear();
}

void GameWorld::ProcessActorsInput()
{
    for (const auto& actorUnique : mActors) {
        actorUnique->ProcessInput();
    }
}

// ステージ再読込に備え、GameWorldの実行時状態を所有権ごと交換する。
void GameWorld::SwapRuntimeState(GameWorld& other) noexcept
{
    using std::swap;
    swap(mPlayers, other.mPlayers);
    swap(mActors, other.mActors);
    swap(mStages, other.mStages);
    swap(mStagesUnique, other.mStagesUnique);
    swap(mCurrentStage, other.mCurrentStage);
    swap(mCurrentStageNum, other.mCurrentStageNum);
    swap(
        mEnemyUpdatePriorityRefreshRemainingSeconds,
        other.mEnemyUpdatePriorityRefreshRemainingSeconds);
}

void GameWorld::ProcessPlayerInput(Player* player)
{
    if (player) {
        player->ProcessInput();
    }
}

void GameWorld::UpdateActors(float deltaTime)
{
    // 敵が多い場合の負荷を抑えるため、一定間隔で
    // プレイヤーに近い敵をフルレート更新対象として選び直す。
    mEnemyUpdatePriorityRefreshRemainingSeconds -=
        std::max(0.0f, deltaTime);

    if (mEnemyUpdatePriorityRefreshRemainingSeconds <= 0.0f) {
        RefreshEnemyUpdatePriorities();
        mEnemyUpdatePriorityRefreshRemainingSeconds =
            enemyUpdatePriorityRefreshIntervalSeconds;
    }

    for (const auto& actorUnique : mActors) {
        actorUnique->Update(deltaTime);
    }
}

void GameWorld::RefreshEnemyUpdatePriorities()
{
    std::vector<EnemyDistance> enemyDistances;
    enemyDistances.reserve(mActors.size());

    for (const std::unique_ptr<Actor>& actor : mActors) {
        Enemy* enemy = dynamic_cast<Enemy*>(actor.get());
        if (!enemy) {
            continue;
        }

        enemy->SetShouldUseFullRateUpdate(false);
        if (!enemy->GetIsActive() || !enemy->IsAlive()) {
            continue;
        }

        float nearestPlayerDistanceSquared =
            std::numeric_limits<float>::max();
        for (const Player* player : mPlayers) {
            if (!player ||
                !player->GetIsActive() ||
                !player->IsAlive()) {
                continue;
            }

            const glm::vec3 enemyToPlayer =
                player->GetPos() - enemy->GetPos();
            nearestPlayerDistanceSquared = std::min(
                nearestPlayerDistanceSquared,
                glm::dot(enemyToPlayer, enemyToPlayer));
        }

        if (nearestPlayerDistanceSquared <
            std::numeric_limits<float>::max()) {
            enemyDistances.push_back(
                {enemy, nearestPlayerDistanceSquared});
        }
    }

    const std::size_t selectedEnemyCount =
        std::min(fullRateEnemyCount, enemyDistances.size());
    // 全敵の並び替えは不要なため、近い敵だけを部分的にソートする。
    std::partial_sort(
        enemyDistances.begin(),
        enemyDistances.begin() + selectedEnemyCount,
        enemyDistances.end(),
        [](const EnemyDistance& left, const EnemyDistance& right) {
            return left.nearestPlayerDistanceSquared <
                right.nearestPlayerDistanceSquared;
        });

    for (std::size_t index = 0;
         index < selectedEnemyCount;
         ++index) {
        enemyDistances[index].enemy->SetShouldUseFullRateUpdate(true);
    }
}

void GameWorld::UpdatePlayer(Player* player, float deltaTime)
{
    if (player) {
        player->Update(deltaTime);
    }
}

void GameWorld::RefreshActorProgressVisibility()
{
    for (const auto& actor : mActors) {
        if (actor) {
            actor->RefreshProgressVisibility();
        }
    }
}

Player* GameWorld::FindNearestPlayer(Actor* actor) const
{
    if (!actor) {
        return nullptr;
    }

    Player* nearestPlayer = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    for (Player* player : mPlayers) {
        if (!player || !player->GetIsActive()) {
            continue;
        }

        const glm::vec3 toPlayer = player->GetPos() - actor->GetPos();
        const float dist = glm::dot(toPlayer, toPlayer);

        if (dist < nearestDist) {
            nearestDist = dist;
            nearestPlayer = player;
        }
    }

    return nearestPlayer;
}

bool GameWorld::ChangeStage(int stageNum)
{
    if (stageNum < 0 || stageNum >= static_cast<int>(mStages.size())) {
        return false;
    }

    mCurrentStage = mStages[stageNum];
    mCurrentStageNum = stageNum;
    return true;
}
