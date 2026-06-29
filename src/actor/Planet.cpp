#include "Planet.h"
#include "Game.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Enemy.h"

Planet::Planet(Game* game)
    : Actor(game),
      mStageNum(0),
      mRemainBoatPartsCount(0),
      mColor(1.0f),
      mKey(nullptr),
      mStar(nullptr),
      mCurrentStage(nullptr),
      mRocketSpawnCondition(RocketSpawnCondition::None),
      mPlanetShape(PlanetShape::Normal)
{
}

void Planet::Initialize()
{
    InitRemainBoatPartsCount();
}

void Planet::InitRemainBoatPartsCount()
{
    if (mBoatParts.empty()) {
        return;
    }

    mRemainBoatPartsCount = 0;
    for (auto parts : mBoatParts) {
        if (!parts->GetIsActive()) {
            continue;
        }

        mRemainBoatPartsCount++;
    }
}

void Planet::OnEnemyDead()
{
    bool shouldCheckIsAllEnemiesDead = mRocketSpawnCondition == RocketSpawnCondition::AllEnemiesDead;
    if (!shouldCheckIsAllEnemiesDead) {
        return;
    }

    if (CheckIsAllEnemiesDead()) {
        StartBoatFocus();
    }
}

bool Planet::CheckIsAllEnemiesDead()
{
    for (auto enemy : mEnemies) {
        if (enemy->GetIsDead()) {
            continue;
        }

        return false;
    }
    return true;
}

void Planet::OnBoatPartsObtained()
{
    mRemainBoatPartsCount--;

    bool shouldCheckAllBoatPartsCollected = mRocketSpawnCondition == RocketSpawnCondition::AllBoatPartsCollected;
    if (!shouldCheckAllBoatPartsCollected) {
        return;
    }

    if (CheckIsAllBoatPartsCollected()) {
        StartBoatFocus();
    }
}

void Planet::StartBoatFocus()
{
    for (auto boat : mBoats) {
        boat->StartFocus();
    }
}

bool Planet::CheckIsAllBoatPartsCollected()
{
    for (auto parts : mBoatParts) {
        if (!parts->GetIsActive()) {
            continue;
        }

        return false;
    }
    return true;
}

glm::vec3 Planet::CalculateSurfacePos(float theta, float phi, float height) const
{
    const glm::vec3 dir =
        glm::normalize(glm::vec3(std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta)));

    return mPos + dir * (mRadius + height);
}