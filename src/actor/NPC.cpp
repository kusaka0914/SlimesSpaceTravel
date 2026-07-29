#include "NPC.h"
#include "Game.h"
#include "Player.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>

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

std::vector<std::size_t> NPC::ResolveTalkIndices() const
{
    int selectedStageCondition = -1;

    for (std::size_t index = 0; index < mTalkTexts.size(); ++index) {
        const int stageCondition = GetTalkStageClearCondition(index);
        if (stageCondition >= 0 && mGame &&
            mGame->IsStageCleared(stageCondition)) {
            selectedStageCondition =
                std::max(selectedStageCondition, stageCondition);
        }
    }

    std::vector<std::size_t> resolvedIndices;
    for (std::size_t index = 0; index < mTalkTexts.size(); ++index) {
        if (GetTalkStageClearCondition(index) == selectedStageCondition) {
            resolvedIndices.emplace_back(index);
        }
    }
    return resolvedIndices;
}

std::vector<std::string> NPC::GetResolvedTalkTexts() const
{
    std::vector<std::string> resolvedTexts;
    for (std::size_t sourceIndex : ResolveTalkIndices()) {
        resolvedTexts.emplace_back(mTalkTexts[sourceIndex]);
    }
    return resolvedTexts;
}

const NPCTalkCameraFocusTarget*
NPC::GetResolvedTalkCameraFocusTarget(std::size_t resolvedIndex) const
{
    const std::vector<std::size_t> indices = ResolveTalkIndices();
    if (resolvedIndex >= indices.size()) {
        return nullptr;
    }
    return GetTalkCameraFocusTarget(indices[resolvedIndex]);
}

const std::vector<RubyTextSegment>&
NPC::GetResolvedTalkRubySegments(std::size_t resolvedIndex) const
{
    static const std::vector<RubyTextSegment> emptySegments;
    const std::vector<std::size_t> indices = ResolveTalkIndices();
    if (resolvedIndex >= indices.size()) {
        return emptySegments;
    }
    return GetTalkRubySegments(indices[resolvedIndex]);
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

void NPC::SetTalkProximityMessageText(
    std::size_t index,
    const std::string& text)
{
    if (index >= mTalkTexts.size()) {
        return;
    }
    if (mTalkProximityMessageTexts.size() < mTalkTexts.size()) {
        mTalkProximityMessageTexts.resize(mTalkTexts.size());
    }

    std::string& messageText = mTalkProximityMessageTexts[index];
    messageText = text;
    std::replace(
        messageText.begin(),
        messageText.end(),
        '\r',
        ' ');
    std::replace(
        messageText.begin(),
        messageText.end(),
        '\n',
        ' ');

    std::size_t escapedNewline = std::string::npos;
    while ((escapedNewline = messageText.find("\\n")) !=
           std::string::npos) {
        messageText.replace(escapedNewline, 2, " ");
    }
}

const std::string& NPC::GetResolvedProximityMessageText() const
{
    static const std::string emptyText;
    const std::vector<std::size_t> indices = ResolveTalkIndices();

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        const std::string& text = GetTalkProximityMessageText(*it);
        if (!text.empty()) {
            return text;
        }
    }
    return emptyText;
}

void NPC::SetProximityMessageRange(float range)
{
    mProximityMessageRange = std::max(0.1f, range);
}

void NPC::SetProximityMessageHeight(float height)
{
    mProximityMessageHeight = std::max(0.0f, height);
}

void NPC::SetProximityMessageScale(float scale)
{
    mProximityMessageScale = std::max(0.1f, scale);
}

bool NPC::CanStartRegularTalk() const
{
    if (mProximityMessageMode == NPCProximityMessageMode::Always) {
        return false;
    }
    return mProximityMessageMode != NPCProximityMessageMode::AfterTalk ||
           !mHasTalkedThisVisit;
}

bool NPC::ShouldShowProximityMessage() const
{
    if (GetResolvedProximityMessageText().empty() ||
        mProximityMessageMode == NPCProximityMessageMode::Disabled) {
        return false;
    }

    const bool isMessageActive =
        mProximityMessageMode == NPCProximityMessageMode::Always ||
        (mProximityMessageMode == NPCProximityMessageMode::AfterTalk &&
         mHasTalkedThisVisit);
    if (!isMessageActive && !(mGame && mGame->GetIsDebugEditorShowing())) {
        return false;
    }

    if (mGame && mGame->GetIsDebugEditorShowing()) {
        return GetIsEditorSelected();
    }

    if (!mGame) {
        return false;
    }
    if (!mGame->GetSceneSystem() ||
        !mGame->GetSceneSystem()->IsPlaying()) {
        return false;
    }

    for (const Player* player : mGame->GetPlayers()) {
        if (!player || !player->GetIsActive()) {
            continue;
        }

        if (glm::length(player->GetPos() - mPos) <= mProximityMessageRange) {
            return true;
        }
    }
    return false;
}

void NPC::LookNearestPlayer(float deltaTime)
{
    const Player* nearestPlayer = mGame->FindNearestPlayer(this);
    const glm::vec3 toNearestPlayer = glm::normalize(nearestPlayer->GetPos() - mPos);

    constexpr float turnSpeed = 5.0f;
    const float t = 1.0f - std::exp(-turnSpeed * deltaTime);

    mFacingForwardVec = glm::normalize(glm::mix(mFacingForwardVec, toNearestPlayer, t));
    SetFacingYaw(mGame->GetMathUtils()->GetYawFromDirection(mUpVec, mFacingForwardVec) + 3.14159265f);
}

void NPC::CheckTalkable()
{
    const std::vector<Player*>& players = mGame->GetPlayers();

    mIsTalkable = false;

    for (Player* player : players) {
        if (!player) {
            continue;
        }

        if (CanStartRegularTalk() && IsPlayerInTalkableRange(player)) {
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
