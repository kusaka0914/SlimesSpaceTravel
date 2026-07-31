#include "actor/planet/PlanetProgressController.h"

#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Enemy.h"
#include "actor/planet/PlanetActorRegistry.h"

PlanetProgressController::PlanetProgressController()
    : mRemainBoatPartsCount(0),
      mRocketSpawnCondition(RocketSpawnCondition::None)
{
}

void PlanetProgressController::Initialize(const PlanetActorRegistry& actorRegistry)
{
    InitRemainBoatPartsCount(actorRegistry);
}

void PlanetProgressController::SetRocketSpawnCondition(const std::string& rocketSpawnCondition)
{
    if (rocketSpawnCondition == "AllEnemiesDead") {
        mRocketSpawnCondition = RocketSpawnCondition::AllEnemiesDead;
    } else if (rocketSpawnCondition == "AllBoatPartsCollected") {
        mRocketSpawnCondition = RocketSpawnCondition::AllBoatPartsCollected;
    } else {
        mRocketSpawnCondition = RocketSpawnCondition::None;
    }
}

std::string PlanetProgressController::GetRocketSpawnCondition() const
{
    switch (mRocketSpawnCondition) {
    case RocketSpawnCondition::AllEnemiesDead:
        return "AllEnemiesDead";
    case RocketSpawnCondition::AllBoatPartsCollected:
        return "AllBoatPartsCollected";
    case RocketSpawnCondition::None:
    default:
        return "";
    }
}

void PlanetProgressController::InitRemainBoatPartsCount(const PlanetActorRegistry& actorRegistry)
{
    mRemainBoatPartsCount = 0;

    for (BoatParts* parts : actorRegistry.GetBoatParts()) {
        if (!parts || !parts->GetIsActive()) {
            continue;
        }

        mRemainBoatPartsCount++;
    }
}

void PlanetProgressController::OnEnemyDead(const PlanetActorRegistry& actorRegistry)
{
    const bool shouldCheckIsAllEnemiesDead = mRocketSpawnCondition == RocketSpawnCondition::AllEnemiesDead;
    if (!shouldCheckIsAllEnemiesDead) {
        return;
    }

    if (CheckIsAllEnemiesDead(actorRegistry)) {
        StartBoatFocus(actorRegistry);
    }
}

bool PlanetProgressController::CheckIsAllEnemiesDead(const PlanetActorRegistry& actorRegistry) const
{
    for (Enemy* enemy : actorRegistry.GetEnemies()) {
        if (!enemy || !enemy->GetIsActive() ||
            enemy->GetIsDead()) {
            continue;
        }

        return false;
    }

    return true;
}

void PlanetProgressController::OnBoatPartsObtained(const PlanetActorRegistry& actorRegistry)
{
    mRemainBoatPartsCount--;

    const bool shouldCheckAllBoatPartsCollected = mRocketSpawnCondition == RocketSpawnCondition::AllBoatPartsCollected;
    if (!shouldCheckAllBoatPartsCollected) {
        return;
    }

    if (CheckIsAllBoatPartsCollected(actorRegistry)) {
        StartBoatFocus(actorRegistry);
    }
}

bool PlanetProgressController::CheckIsAllBoatPartsCollected(const PlanetActorRegistry& actorRegistry) const
{
    for (BoatParts* parts : actorRegistry.GetBoatParts()) {
        if (!parts || !parts->GetIsActive()) {
            continue;
        }

        return false;
    }

    return true;
}

void PlanetProgressController::StartBoatFocus(const PlanetActorRegistry& actorRegistry) const
{
    for (Boat* boat : actorRegistry.GetBoats()) {
        if (!boat || !boat->GetIsActive()) {
            continue;
        }

        boat->StartFocus();
    }
}
