#include "system/EnemyJewelDropSystem.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/JewelItem.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"

#include <algorithm>
#include <array>
#include <memory>
#include <random>

namespace {
constexpr std::array<float, 3> jewelDropChances = {
    0.15f,
    0.30f,
    1.00f,
};
}

EnemyJewelDropSystem::EnemyJewelDropSystem(Game* game)
    : mGame(game)
{
}

void EnemyJewelDropSystem::RequestDrop(
    const Enemy& defeatedEnemy)
{
    Planet* planet = defeatedEnemy.GetCurrentPlanet();
    if (!mGame ||
        !planet ||
        defeatedEnemy.GetIsBoss() ||
        !ShouldDropJewel()) {
        return;
    }

    PendingDrop pendingDrop;
    pendingDrop.planet = planet;
    pendingDrop.position = defeatedEnemy.GetLastGroundedPosition();
    pendingDrop.upDirection =
        defeatedEnemy.GetLastGroundedUpDirection();
    mPendingDrops.emplace_back(pendingDrop);
}

bool EnemyJewelDropSystem::ShouldDropJewel()
{
    const std::size_t chanceIndex =
        std::min(
            static_cast<std::size_t>(mFailedDropCount),
            jewelDropChances.size() - 1);

    thread_local std::mt19937 randomEngine(
        std::random_device{}());
    std::bernoulli_distribution dropRoll(
        jewelDropChances[chanceIndex]);
    const bool shouldDrop = dropRoll(randomEngine);
    if (shouldDrop) {
        mFailedDropCount = 0;
        return true;
    }

    ++mFailedDropCount;
    return false;
}

void EnemyJewelDropSystem::SpawnPendingDrops()
{
    if (!mGame || !mGame->GetMeshLoadSystem()) {
        mPendingDrops.clear();
        return;
    }

    for (const PendingDrop& pendingDrop : mPendingDrops) {
        if (!pendingDrop.planet) {
            continue;
        }

        auto jewelItem =
            std::make_unique<JewelItem>(mGame);
        jewelItem->SetCurrentPlanet(pendingDrop.planet);
        jewelItem->SetPos(
            pendingDrop.position +
            pendingDrop.upDirection * 0.15f);
        jewelItem->SetUpVec(pendingDrop.upDirection);
        jewelItem->Initialize();
        mGame->GetMeshLoadSystem()->SetActorMesh(
            jewelItem.get());

        mRuntimeItems.emplace_back(jewelItem.get());
        mGame->AddActor(std::move(jewelItem));
    }

    mPendingDrops.clear();
}

void EnemyJewelDropSystem::ClearRuntimeDrops()
{
    mPendingDrops.clear();
    mRuntimeItems.clear();
}
