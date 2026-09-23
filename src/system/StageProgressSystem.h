#pragma once

#include <filesystem>
#include <set>
#include <string>

class StageProgressSystem {
public:
    StageProgressSystem();
    explicit StageProgressSystem(std::filesystem::path savePath);
    StageProgressSystem(
        std::filesystem::path savePath,
        bool isPersistenceEnabled);

    bool Load();
    bool Save() const;
    void Reset();

    bool IsStageCleared(int stageNum) const;
    bool MarkStageCleared(int stageNum);
    bool SetStageCleared(int stageNum, bool isCleared);
    bool HasShownConversation(const std::string& conversationId) const;
    bool MarkConversationShown(const std::string& conversationId);




    bool HasCompletedEndingRoll() const { return mHasCompletedEndingRoll; }
    bool SetEndingRollCompleted(bool completed = true);

    bool HasSelectedPlayerControlStyle() const
    {
        return mHasSelectedPlayerControlStyle;
    }
    bool IsAssistControlStyleSelected() const
    {
        return mIsAssistControlStyleSelected;
    }
    bool SetSelectedPlayerControlStyle(bool isAssistControlStyle);

private:
    std::filesystem::path mSavePath;
    std::set<int> mClearedStages;
    std::set<std::string> mShownConversationIds;
    bool mHasCompletedEndingRoll = false;
    bool mHasSelectedPlayerControlStyle = false;
    bool mIsAssistControlStyleSelected = false;
    bool mIsPersistenceEnabled = true;
};
