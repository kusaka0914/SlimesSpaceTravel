#pragma once

#include <set>
#include <string>

class StageProgressSystem {
public:
    explicit StageProgressSystem(
        std::string savePath = "../assets/data/save/stage_progress.yaml");

    bool Load();
    bool Save() const;

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
    std::string mSavePath;
    std::set<int> mClearedStages;
    std::set<std::string> mShownConversationIds;
    bool mHasCompletedEndingRoll = false;
    bool mHasSelectedPlayerControlStyle = false;
    bool mIsAssistControlStyleSelected = false;
};
