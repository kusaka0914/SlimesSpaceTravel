#include "system/GameWorld.h"

#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Player.h"

#include <limits>
#include <utility>
#include <glm/glm.hpp>

void GameWorld::CreateStages(int stageCount)
{
    mStages.clear();
    mStagesUnique.clear();
    mCurrentStage = nullptr;
    mCurrentStageNum = 0;

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

void GameWorld::SwapRuntimeState(GameWorld& other) noexcept
{
    using std::swap;
    swap(mPlayers, other.mPlayers);
    swap(mActors, other.mActors);
    swap(mStages, other.mStages);
    swap(mStagesUnique, other.mStagesUnique);
    swap(mCurrentStage, other.mCurrentStage);
    swap(mCurrentStageNum, other.mCurrentStageNum);
}

void GameWorld::ProcessPlayerInput(Player* player)
{
    if (player) {
        player->ProcessInput();
    }
}

void GameWorld::UpdateActors(float deltaTime)
{
    for (const auto& actorUnique : mActors) {
        actorUnique->Update(deltaTime);
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
