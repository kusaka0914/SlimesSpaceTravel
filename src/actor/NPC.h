#pragma once

#include "actor/CharacterActor.h"
#include "text/RubyText.h"
#include <cstddef>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Game;
class Player;

struct NPCTalkCameraFocusTarget {
    std::string sequenceName;
    int yamlIndex = -1;

    bool IsValid() const { return !sequenceName.empty() && yamlIndex >= 0; }
};

enum class NPCProximityMessageMode {
    Disabled = 0,
    AfterTalk,
    Always
};

class NPC : public CharacterActor {
public:
    NPC(Game* game);

    void ApplyConfig(const std::string& type);

    void UpdateActor(float deltaTime) override;

    void AddTalkTexts(const std::string& talkTexts)
    {
        mTalkTexts.emplace_back(talkTexts);
        mTalkCameraFocusTargets.emplace_back(std::nullopt);
        mTalkRubySegments.emplace_back();
        mTalkStageClearConditions.emplace_back(-1);
        mTalkProximityMessageTexts.emplace_back();
    }
    void SetTalkText(std::size_t index, const std::string& talkText)
    {
        if (index < mTalkTexts.size()) {
            if (mTalkTexts[index] != talkText && index < mTalkRubySegments.size()) {
                mTalkRubySegments[index].clear();
            }
            mTalkTexts[index] = talkText;
        }
    }
    void RemoveTalkText(std::size_t index)
    {
        if (index < mTalkTexts.size()) {
            mTalkTexts.erase(mTalkTexts.begin() + static_cast<std::ptrdiff_t>(index));
            if (index < mTalkCameraFocusTargets.size()) {
                mTalkCameraFocusTargets.erase(
                    mTalkCameraFocusTargets.begin() + static_cast<std::ptrdiff_t>(index));
            }
            if (index < mTalkRubySegments.size()) {
                mTalkRubySegments.erase(
                    mTalkRubySegments.begin() + static_cast<std::ptrdiff_t>(index));
            }
            if (index < mTalkStageClearConditions.size()) {
                mTalkStageClearConditions.erase(
                    mTalkStageClearConditions.begin() +
                    static_cast<std::ptrdiff_t>(index));
            }
            if (index < mTalkProximityMessageTexts.size()) {
                mTalkProximityMessageTexts.erase(
                    mTalkProximityMessageTexts.begin() +
                    static_cast<std::ptrdiff_t>(index));
            }
        }
    }
    void SetTalkStageClearCondition(std::size_t index, int stageNum)
    {
        if (index >= mTalkTexts.size()) {
            return;
        }
        if (mTalkStageClearConditions.size() < mTalkTexts.size()) {
            mTalkStageClearConditions.resize(mTalkTexts.size(), -1);
        }
        mTalkStageClearConditions[index] = stageNum >= 0 ? stageNum : -1;
    }
    void SetTalkCameraFocusTarget(std::size_t index, const std::string& sequenceName, int yamlIndex)
    {
        if (index >= mTalkTexts.size()) {
            return;
        }
        if (mTalkCameraFocusTargets.size() < mTalkTexts.size()) {
            mTalkCameraFocusTargets.resize(mTalkTexts.size());
        }
        mTalkCameraFocusTargets[index] = NPCTalkCameraFocusTarget{sequenceName, yamlIndex};
    }
    void ClearTalkCameraFocusTarget(std::size_t index)
    {
        if (index < mTalkCameraFocusTargets.size()) {
            mTalkCameraFocusTargets[index] = std::nullopt;
        }
    }
    void SetTalkRubySegments(std::size_t index, std::vector<RubyTextSegment> segments)
    {
        if (index >= mTalkTexts.size()) {
            return;
        }
        if (mTalkRubySegments.size() < mTalkTexts.size()) {
            mTalkRubySegments.resize(mTalkTexts.size());
        }
        mTalkRubySegments[index] = std::move(segments);
    }
    void SetTalkRubyReading(std::size_t talkIndex, std::size_t segmentIndex,
                            const std::string& reading)
    {
        if (talkIndex < mTalkRubySegments.size() &&
            segmentIndex < mTalkRubySegments[talkIndex].size()) {
            mTalkRubySegments[talkIndex][segmentIndex].reading = reading;
        }
    }
    void ClearTalkRubySegments(std::size_t index)
    {
        if (index < mTalkRubySegments.size()) {
            mTalkRubySegments[index].clear();
        }
    }

    void SetName(const std::string& name) { mName = name; }
    void SetProximityMessageMode(NPCProximityMessageMode mode)
    {
        mProximityMessageMode = mode;
    }
    void SetTalkProximityMessageText(
        std::size_t index,
        const std::string& text);
    void SetProximityMessageRange(float range);
    void SetProximityMessageHeight(float height);
    void SetProximityMessageScale(float scale);
    void MarkTalkCompletedThisVisit() { mHasTalkedThisVisit = true; }

    bool GetIsTalkable() const { return mIsTalkable; }
    const std::string& GetName() const { return mName; }
    const std::vector<std::string>& GetTalkTexts() const { return mTalkTexts; }
    int GetTalkStageClearCondition(std::size_t index) const
    {
        return index < mTalkStageClearConditions.size()
                   ? mTalkStageClearConditions[index]
                   : -1;
    }
    std::vector<std::string> GetResolvedTalkTexts() const;
    const NPCTalkCameraFocusTarget*
    GetResolvedTalkCameraFocusTarget(std::size_t resolvedIndex) const;
    const std::vector<RubyTextSegment>&
    GetResolvedTalkRubySegments(std::size_t resolvedIndex) const;
    const NPCTalkCameraFocusTarget* GetTalkCameraFocusTarget(std::size_t index) const
    {
        if (index >= mTalkCameraFocusTargets.size() || !mTalkCameraFocusTargets[index]) {
            return nullptr;
        }
        return &*mTalkCameraFocusTargets[index];
    }
    const std::vector<RubyTextSegment>& GetTalkRubySegments(std::size_t index) const
    {
        static const std::vector<RubyTextSegment> emptySegments;
        return index < mTalkRubySegments.size()
                   ? mTalkRubySegments[index]
                   : emptySegments;
    }
    bool HasValidTalkRuby(std::size_t index) const
    {
        return index < mTalkTexts.size() &&
               index < mTalkRubySegments.size() &&
               !mTalkRubySegments[index].empty() &&
               JoinRubyBaseText(mTalkRubySegments[index]) == mTalkTexts[index];
    }
    NPCProximityMessageMode GetProximityMessageMode() const
    {
        return mProximityMessageMode;
    }
    const std::string& GetTalkProximityMessageText(
        std::size_t index) const
    {
        static const std::string emptyText;
        return index < mTalkProximityMessageTexts.size()
                   ? mTalkProximityMessageTexts[index]
                   : emptyText;
    }
    const std::string& GetResolvedProximityMessageText() const;
    float GetProximityMessageRange() const { return mProximityMessageRange; }
    float GetProximityMessageHeight() const { return mProximityMessageHeight; }
    float GetProximityMessageScale() const { return mProximityMessageScale; }
    bool GetHasTalkedThisVisit() const { return mHasTalkedThisVisit; }
    bool CanStartRegularTalk() const;
    bool ShouldShowProximityMessage() const;

private:
    std::vector<std::size_t> ResolveTalkIndices() const;
    void LookNearestPlayer(float deltaTime);
    void CheckTalkable();

    Player* FindNearestPlayer() const;
    bool IsPlayerInTalkableRange(Player* player) const;

private:
    bool mIsTalkable;
    std::string mName;
    std::vector<std::string> mTalkTexts;
    std::vector<std::optional<NPCTalkCameraFocusTarget>> mTalkCameraFocusTargets;
    std::vector<std::vector<RubyTextSegment>> mTalkRubySegments;
    std::vector<int> mTalkStageClearConditions;
    NPCProximityMessageMode mProximityMessageMode =
        NPCProximityMessageMode::Disabled;
    std::vector<std::string> mTalkProximityMessageTexts;
    float mProximityMessageRange = 3.0f;
    float mProximityMessageHeight = 1.8f;
    float mProximityMessageScale = 1.0f;
    bool mHasTalkedThisVisit = false;
};
