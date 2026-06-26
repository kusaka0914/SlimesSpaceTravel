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

namespace {
glm::vec3 ReadVec3(const YAML::Node& node, const char* key, const glm::vec3& defaultValue)
{
    if (!node[key] || !node[key].IsSequence() || node[key].size() < 3) {
        return defaultValue;
    }

    return glm::vec3(node[key][0] ? node[key][0].as<float>() : defaultValue.x,
                     node[key][1] ? node[key][1].as<float>() : defaultValue.y,
                     node[key][2] ? node[key][2].as<float>() : defaultValue.z);
}

glm::vec4 ReadVec4(const YAML::Node& node, const char* key, const glm::vec4& defaultValue)
{
    if (!node[key] || !node[key].IsSequence() || node[key].size() < 4) {
        return defaultValue;
    }

    return glm::vec4(node[key][0] ? node[key][0].as<float>() : defaultValue.x,
                     node[key][1] ? node[key][1].as<float>() : defaultValue.y,
                     node[key][2] ? node[key][2].as<float>() : defaultValue.z,
                     node[key][3] ? node[key][3].as<float>() : defaultValue.w);
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}
} // namespace

void Planet::ApplyConfig(const YAML::Node& node)
{
    const glm::vec3 center = ReadVec3(node, "center", glm::vec3(0.0f));
    SetPos(center);

    const glm::vec3 scale = ReadVec3(node, "scale", glm::vec3(1.0f));
    SetScale(scale);
    SetRadius(scale.x);

    const glm::vec4 color = ReadVec4(node, "color", glm::vec4(1.0f));
    SetColor(color);

    const std::string modelPath = ReadString(node, "model", "planet.obj");
    SetModelPath(modelPath);

    const std::string shape = ReadString(node, "shape", "Sphere");
    SetPlanetShape(shape);

    const int stageNum = ReadInt(node, "stageNum", 0);
    SetStageNum(stageNum);

    const std::string rocketSpawnCondition = ReadString(node, "rocketSpawnCondition", "");
    SetRocketSpawnCondition(rocketSpawnCondition);
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