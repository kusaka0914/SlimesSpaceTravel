#include "NPC.h"
#include "Game.h"
#include "Player.h"
#include "utils/MathUtils.h"

#include <yaml-cpp/yaml.h>

NPC::NPC(Game* game)
    : CharacterActor(game),
      mIsTalkable(false)
{
    mShouldJudgeLanding = true;
}

void NPC::ApplyConfig(const std::string& type)
{
    YAML::Node npcRoot = YAML::LoadFile("../assets/data/actor/npcs.yaml");

    if (!npcRoot["npcs"] || !npcRoot["npcs"].IsSequence()) {
        return;
    }

    for (const YAML::Node& npcNode : npcRoot["npcs"]) {
        const std::string npcType = npcNode["type"] ? npcNode["type"].as<std::string>() : "";

        if (type != npcType) {
            continue;
        }

        const std::string modelPath = npcNode["modelPath"] ? npcNode["modelPath"].as<std::string>() : "npc.obj";
        SetModelPath(modelPath);

        const float scale = npcNode["scale"] ? npcNode["scale"].as<float>() : 0.25f;
        SetScale(glm::vec3(scale));

        return;
    }
}

void NPC::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);
    LookNearestPlayer(deltaTime);
    CheckTalkable();

    if (!mOnGround) {
        ApplyGravity(deltaTime);
    }
}

void NPC::LookNearestPlayer(float deltaTime)
{
    const Player* nearestPlayer = mGame->FindNearestPlayer(this);
    const glm::vec3 toNearestPlayer = glm::normalize(nearestPlayer->GetPos() - mPos);

    constexpr float turnSpeed = 5.0f;
    const float t = 1.0f - std::exp(-turnSpeed * deltaTime);

    mFacingForwardVec = glm::normalize(glm::mix(mFacingForwardVec, toNearestPlayer, t));
    mFacingYaw = mGame->GetMathUtils()->GetYawFromDirection(mUpVec, mFacingForwardVec) + 3.14159265f;
}

void NPC::CheckTalkable()
{
    const std::vector<Player*>& players = mGame->GetPlayers();

    mIsTalkable = false;

    for (Player* player : players) {
        if (!player) {
            continue;
        }

        if (IsPlayerInTalkableRange(player)) {
            mIsTalkable = true;
            player->SetTalkableNPC(this);
        } else if (player->GetTalkableNPC() == this) {
            player->SetTalkableNPC(nullptr);
        }
    }
}

bool NPC::IsPlayerInTalkableRange(Player* player) const
{
    const float toPlayerDist = glm::length(player->GetPos() - mPos);
    constexpr float talkableRangeMargin = 0.5f;
    const float talkableRange = mRadius + talkableRangeMargin;

    return toPlayerDist <= talkableRange;
}