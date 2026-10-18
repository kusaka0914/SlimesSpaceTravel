#include "actor/NPC.h"

#include "app/Game.h"
#include "actor/Player.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <cmath>
#include <yaml-cpp/yaml.h>

NPC::NPC(Game* game)
    : CharacterActor(game),
      mIsTalkable(false)
{
    mShouldJudgeLanding = true;
}

void NPC::ApplyConfig(const YAML::Node& npcRoot, const std::string& type)
{
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
    const int selectedStageCondition = ResolveTalkStageClearCondition();

    std::vector<std::size_t> resolvedIndices;
    for (std::size_t index = 0; index < mTalkData.GetTalkTexts().size(); ++index) {
        if (GetTalkStageClearCondition(index) == selectedStageCondition) {
            resolvedIndices.emplace_back(index);
        }
    }
    return resolvedIndices;
}

int NPC::ResolveTalkStageClearCondition() const
{
    // クリア済みの最大ステージ番号の会話群を選ぶ。該当しなければ通常会話（-1）を使う。
    int selectedStageCondition = -1;

    for (std::size_t index = 0; index < mTalkData.GetTalkTexts().size(); ++index) {
        const int stageCondition = GetTalkStageClearCondition(index);
        if (stageCondition >= 0 && mGame && mGame->IsStageCleared(stageCondition)) {
            selectedStageCondition = std::max(selectedStageCondition, stageCondition);
        }
    }
    return selectedStageCondition;
}

std::vector<std::string> NPC::GetResolvedTalkTexts() const
{
    std::vector<std::string> resolvedTexts;
    for (std::size_t sourceIndex : ResolveTalkIndices()) {
        resolvedTexts.emplace_back(mTalkData.GetTalkTexts()[sourceIndex]);
    }
    return resolvedTexts;
}

const NPCTalkCameraFocusTarget* NPC::GetResolvedTalkCameraFocusTarget(std::size_t resolvedIndex) const
{
    const std::vector<std::size_t> indices = ResolveTalkIndices();
    if (resolvedIndex >= indices.size()) {
        return nullptr;
    }
    return GetTalkCameraFocusTarget(indices[resolvedIndex]);
}

TalkPageAdvanceCondition NPC::GetResolvedTalkAdvanceCondition(std::size_t resolvedIndex) const
{
    const std::vector<std::size_t> indices = ResolveTalkIndices();
    if (resolvedIndex >= indices.size()) {
        return TalkPageAdvanceCondition::Confirm;
    }
    return GetTalkAdvanceCondition(indices[resolvedIndex]);
}

bool NPC::GetResolvedTalkStartsOpeningAfterPage(std::size_t resolvedIndex) const
{
    const std::optional<std::size_t> sourceIndex = GetResolvedTalkSourceIndex(resolvedIndex);
    return sourceIndex && GetTalkStartsOpeningAfterPage(*sourceIndex);
}

bool NPC::GetResolvedTalkStartsEndingAfterPage(std::size_t resolvedIndex) const
{
    const std::optional<std::size_t> sourceIndex = GetResolvedTalkSourceIndex(resolvedIndex);
    return sourceIndex && GetTalkStartsEndingAfterPage(*sourceIndex);
}

std::optional<std::size_t> NPC::GetResolvedTalkSourceIndex(std::size_t resolvedIndex) const
{
    const std::vector<std::size_t> indices = ResolveTalkIndices();
    if (resolvedIndex >= indices.size()) {
        return std::nullopt;
    }
    return indices[resolvedIndex];
}

const std::vector<RubyTextSegment>& NPC::GetResolvedTalkRubySegments(std::size_t resolvedIndex) const
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

void NPC::SetTalkProximityMessageText(std::size_t index, const std::string& text)
{
    mTalkData.SetTalkProximityMessageText(index, text);
}

void NPC::SetTalkProximityMessageRubySegments(std::size_t index, std::vector<RubyTextSegment> segments)
{
    mTalkData.SetTalkProximityMessageRubySegments(index, std::move(segments));
}

void NPC::SetTalkProximityMessageRubyReading(std::size_t talkIndex, std::size_t segmentIndex,
                                             const std::string& reading)
{
    mTalkData.SetTalkProximityMessageRubyReading(talkIndex, segmentIndex, reading);
}

void NPC::ClearTalkProximityMessageRubySegments(std::size_t index)
{
    mTalkData.ClearTalkProximityMessageRubySegments(index);
}

std::optional<std::size_t> NPC::ResolveProximityMessageTalkIndex() const
{
    const std::vector<std::size_t> indices = ResolveTalkIndices();

    // 条件に合うページを末尾から調べ、空でない最後の頭上メッセージを採用する。
    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        if (!GetTalkProximityMessageText(*it).empty()) {
            return *it;
        }
    }
    return std::nullopt;
}

const std::string& NPC::GetResolvedProximityMessageText() const
{
    static const std::string emptyText;
    const std::optional<std::size_t> talkIndex = ResolveProximityMessageTalkIndex();
    if (!talkIndex) {
        return emptyText;
    }
    return GetTalkProximityMessageText(*talkIndex);
}

const std::vector<RubyTextSegment>& NPC::GetResolvedProximityMessageRubySegments() const
{
    static const std::vector<RubyTextSegment> emptySegments;
    const std::optional<std::size_t> talkIndex = ResolveProximityMessageTalkIndex();
    if (!talkIndex || !HasValidTalkProximityMessageRuby(*talkIndex)) {
        return emptySegments;
    }
    return GetTalkProximityMessageRubySegments(*talkIndex);
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
    return mProximityMessageMode != NPCProximityMessageMode::AfterTalk || !mHasTalkedThisVisit;
}

bool NPC::ShouldShowProximityMessage() const
{
    if (GetResolvedProximityMessageText().empty() || mProximityMessageMode == NPCProximityMessageMode::Disabled) {
        return false;
    }

    const bool isMessageActive = mProximityMessageMode == NPCProximityMessageMode::Always ||
                                 (mProximityMessageMode == NPCProximityMessageMode::AfterTalk && mHasTalkedThisVisit);

    // 編集中は進行条件を満たさなくても、選択中のNPCのメッセージを確認できる。
    if (!isMessageActive && !(mGame && mGame->GetIsDebugEditorShowing())) {
        return false;
    }

    if (mGame && mGame->GetIsDebugEditorShowing()) {
        return GetIsEditorSelected();
    }

    if (!mGame) {
        return false;
    }
    if (!mGame->GetSceneSystem() || !mGame->GetSceneSystem()->IsPlaying()) {
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
    if (!nearestPlayer) {
        return;
    }

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
