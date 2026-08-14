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

enum class TalkPageAdvanceCondition {
    Confirm = 0,
    PlayerSwitch,
    Jump
};

const char* GetTalkPageAdvanceConditionId(
    TalkPageAdvanceCondition condition);
TalkPageAdvanceCondition ParseTalkPageAdvanceConditionId(
    const std::string& conditionId);

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
        mTalkProximityMessageRubySegments.emplace_back();
        mTalkAdvanceConditions.emplace_back(
            TalkPageAdvanceCondition::Confirm);
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
            if (index < mTalkProximityMessageRubySegments.size()) {
                mTalkProximityMessageRubySegments.erase(
                    mTalkProximityMessageRubySegments.begin() +
                    static_cast<std::ptrdiff_t>(index));
            }
            if (index < mTalkAdvanceConditions.size()) {
                mTalkAdvanceConditions.erase(
                    mTalkAdvanceConditions.begin() +
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
    void SetTalkAdvanceCondition(
        std::size_t index,
        TalkPageAdvanceCondition condition)
    {
        if (index >= mTalkTexts.size()) {
            return;
        }
        if (mTalkAdvanceConditions.size() < mTalkTexts.size()) {
            mTalkAdvanceConditions.resize(
                mTalkTexts.size(),
                TalkPageAdvanceCondition::Confirm);
        }
        mTalkAdvanceConditions[index] = condition;
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
    void SetTalkProximityMessageRubySegments(
        std::size_t index,
        std::vector<RubyTextSegment> segments);
    void SetTalkProximityMessageRubyReading(
        std::size_t talkIndex,
        std::size_t segmentIndex,
        const std::string& reading);
    void ClearTalkProximityMessageRubySegments(std::size_t index);
    void SetProximityMessageRange(float range);
    void SetProximityMessageHeight(float height);
    void SetProximityMessageScale(float scale);
    void MarkTalkCompletedThisVisit() { mHasTalkedThisVisit = true; }
    void SetForcesTalkOnArrival(bool forcesTalkOnArrival)
    {
        mForcesTalkOnArrival = forcesTalkOnArrival;
    }

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
    TalkPageAdvanceCondition
    GetResolvedTalkAdvanceCondition(std::size_t resolvedIndex) const;
    const std::vector<RubyTextSegment>&
    GetResolvedTalkRubySegments(std::size_t resolvedIndex) const;
    const NPCTalkCameraFocusTarget* GetTalkCameraFocusTarget(std::size_t index) const
    {
        if (index >= mTalkCameraFocusTargets.size() || !mTalkCameraFocusTargets[index]) {
            return nullptr;
        }
        return &*mTalkCameraFocusTargets[index];
    }
    TalkPageAdvanceCondition
    GetTalkAdvanceCondition(std::size_t index) const
    {
        return index < mTalkAdvanceConditions.size()
                   ? mTalkAdvanceConditions[index]
                   : TalkPageAdvanceCondition::Confirm;
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
    const std::vector<RubyTextSegment>&
    GetResolvedProximityMessageRubySegments() const;
    const std::vector<RubyTextSegment>&
    GetTalkProximityMessageRubySegments(std::size_t index) const
    {
        static const std::vector<RubyTextSegment> emptySegments;
        return index < mTalkProximityMessageRubySegments.size()
                   ? mTalkProximityMessageRubySegments[index]
                   : emptySegments;
    }
    bool HasValidTalkProximityMessageRuby(std::size_t index) const
    {
        return index < mTalkProximityMessageTexts.size() &&
               index < mTalkProximityMessageRubySegments.size() &&
               !mTalkProximityMessageRubySegments[index].empty() &&
               JoinRubyBaseText(
                   mTalkProximityMessageRubySegments[index]) ==
                   mTalkProximityMessageTexts[index];
    }
    float GetProximityMessageRange() const { return mProximityMessageRange; }
    float GetProximityMessageHeight() const { return mProximityMessageHeight; }
    float GetProximityMessageScale() const { return mProximityMessageScale; }
    bool GetHasTalkedThisVisit() const { return mHasTalkedThisVisit; }
    bool GetForcesTalkOnArrival() const { return mForcesTalkOnArrival; }
    int ResolveTalkStageClearCondition() const;
    bool CanStartRegularTalk() const;
    bool ShouldShowProximityMessage() const;
    virtual bool ShouldUseTalkCamera() const { return true; }
    virtual bool ShouldFacePlayerDuringTalk() const { return true; }

private:
    std::vector<std::size_t> ResolveTalkIndices() const;
    std::optional<std::size_t>
    ResolveProximityMessageTalkIndex() const;
    void LookNearestPlayer(float deltaTime);
    void CheckTalkable();

    Player* FindNearestPlayer() const;
    bool IsPlayerInTalkableRange(Player* player) const;

private:
    bool mIsTalkable;
    std::string mName;
    std::vector<std::string> mTalkTexts;
    std::vector<std::optional<NPCTalkCameraFocusTarget>> mTalkCameraFocusTargets;
    std::vector<TalkPageAdvanceCondition> mTalkAdvanceConditions;
    std::vector<std::vector<RubyTextSegment>> mTalkRubySegments;
    std::vector<int> mTalkStageClearConditions;
    NPCProximityMessageMode mProximityMessageMode =
        NPCProximityMessageMode::Disabled;
    std::vector<std::string> mTalkProximityMessageTexts;
    std::vector<std::vector<RubyTextSegment>>
        mTalkProximityMessageRubySegments;
    float mProximityMessageRange = 3.0f;
    float mProximityMessageHeight = 1.8f;
    float mProximityMessageScale = 1.0f;
    bool mHasTalkedThisVisit = false;
    bool mForcesTalkOnArrival = false;
};
