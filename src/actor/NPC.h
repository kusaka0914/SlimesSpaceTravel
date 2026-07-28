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
        }
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

    bool GetIsTalkable() const { return mIsTalkable; }
    const std::string& GetName() const { return mName; }
    const std::vector<std::string>& GetTalkTexts() const { return mTalkTexts; }
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

private:
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
};
