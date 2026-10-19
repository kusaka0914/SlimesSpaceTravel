#include "system/StageProgressSystem.h"

#include "system/UserDataPaths.h"
#include "utils/AtomicFile.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <yaml-cpp/yaml.h>

StageProgressSystem::StageProgressSystem()
    : StageProgressSystem(UserDataPaths::ResolveStageProgressFile())
{
}

StageProgressSystem::StageProgressSystem(std::filesystem::path savePath)
    : StageProgressSystem(std::move(savePath), true)
{
}

StageProgressSystem::StageProgressSystem(std::filesystem::path savePath, bool isPersistenceEnabled)
    : mSavePath(std::move(savePath)),
      mIsPersistenceEnabled(isPersistenceEnabled)
{
}

bool StageProgressSystem::Load()
{
    if (!mIsPersistenceEnabled) {
        Reset();
        return true;
    }

    std::ifstream input(mSavePath);
    if (!input.is_open()) {
        std::error_code error;
        if (!std::filesystem::exists(mSavePath, error) && !error) {
            Reset();
            return true;
        }
        std::cerr << "Failed to open stage progress: " << mSavePath.string() << '\n';
        return false;
    }

    StageProgressSystem loaded(mSavePath, mIsPersistenceEnabled);
    try {
        const YAML::Node root = YAML::Load(input);
        if (!root.IsMap()) {
            return false;
        }

        const YAML::Node clearedStages = root["clearedStages"];
        if (clearedStages && clearedStages.IsSequence()) {
            for (const YAML::Node& stageNode : clearedStages) {
                const int stageNum = stageNode.as<int>();
                if (stageNum >= 0) {
                    loaded.mClearedStages.insert(stageNum);
                }
            }
        }

        const YAML::Node shownConversations = root["shownConversations"];
        if (shownConversations && shownConversations.IsSequence()) {
            for (const YAML::Node& conversationNode : shownConversations) {
                const std::string conversationId = conversationNode.as<std::string>();
                if (!conversationId.empty()) {
                    loaded.mShownConversationIds.insert(conversationId);
                }
            }
        }

        const YAML::Node endingRollCompleted = root["endingRollCompleted"];
        loaded.mHasCompletedEndingRoll = endingRollCompleted && endingRollCompleted.IsScalar()
                                             ? endingRollCompleted.as<bool>()
                                             : loaded.mShownConversationIds.contains("cinematic:ending_roll_completed");

        const YAML::Node playerControlStyle = root["playerControlStyle"];
        if (playerControlStyle && playerControlStyle.IsScalar()) {
            const std::string style = playerControlStyle.as<std::string>();
            if (style == "assist" || style == "standard") {
                loaded.mHasSelectedPlayerControlStyle = true;
                loaded.mIsAssistControlStyleSelected = style == "assist";
            }
        }

    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load stage progress: " << exception.what() << '\n';
        return false;
    }
    *this = std::move(loaded);
    return true;
}

bool StageProgressSystem::Save() const
{
    if (!mIsPersistenceEnabled) {
        return true;
    }

    const std::filesystem::path& savePath = mSavePath;
    std::error_code error;
    if (savePath.has_parent_path()) {
        std::filesystem::create_directories(savePath.parent_path(), error);
        if (error) {
            std::cerr << "Failed to create stage progress directory: " << error.message() << '\n';
            return false;
        }
    }

    YAML::Node root;
    root["clearedStages"] = YAML::Node(YAML::NodeType::Sequence);
    for (int stageNum : mClearedStages) {
        root["clearedStages"].push_back(stageNum);
    }
    root["shownConversations"] = YAML::Node(YAML::NodeType::Sequence);
    for (const std::string& conversationId : mShownConversationIds) {
        root["shownConversations"].push_back(conversationId);
    }
    root["endingRollCompleted"] = mHasCompletedEndingRoll;
    if (mHasSelectedPlayerControlStyle) {
        root["playerControlStyle"] = mIsAssistControlStyleSelected ? "assist" : "standard";
    }

    std::string errorMessage;
    if (!AtomicFile::WriteText(mSavePath, YAML::Dump(root), errorMessage)) {
        std::cerr << "Failed to save stage progress: " << errorMessage << '\n';
        return false;
    }
    return true;
}

void StageProgressSystem::Reset()
{
    mClearedStages.clear();
    mShownConversationIds.clear();
    mHasCompletedEndingRoll = false;
    mHasSelectedPlayerControlStyle = false;
    mIsAssistControlStyleSelected = false;
}

bool StageProgressSystem::IsStageCleared(int stageNum) const
{
    return stageNum >= 0 && mClearedStages.contains(stageNum);
}

ProgressUpdateResult StageProgressSystem::MarkStageCleared(int stageNum)
{
    return SetStageCleared(stageNum, true);
}

ProgressUpdateResult StageProgressSystem::SetStageCleared(int stageNum, bool isCleared)
{
    if (stageNum < 0) {
        return {};
    }

    bool changed = false;
    if (isCleared) {
        changed = mClearedStages.insert(stageNum).second;
    } else {
        changed = mClearedStages.erase(stageNum) > 0;
    }

    return SaveChange(changed);
}

bool StageProgressSystem::HasShownConversation(const std::string& conversationId) const
{
    return !conversationId.empty() && mShownConversationIds.contains(conversationId);
}

ProgressUpdateResult StageProgressSystem::MarkConversationShown(const std::string& conversationId)
{
    if (conversationId.empty()) {
        return {};
    }

    const bool changed = mShownConversationIds.insert(conversationId).second;
    return SaveChange(changed);
}

ProgressUpdateResult StageProgressSystem::SetEndingRollCompleted(bool completed)
{
    if (mHasCompletedEndingRoll == completed) {
        return {};
    }

    mHasCompletedEndingRoll = completed;
    return SaveChange(true);
}

ProgressUpdateResult StageProgressSystem::SetSelectedPlayerControlStyle(bool isAssistControlStyle)
{
    const bool changed = !mHasSelectedPlayerControlStyle || mIsAssistControlStyleSelected != isAssistControlStyle;
    mHasSelectedPlayerControlStyle = true;
    mIsAssistControlStyleSelected = isAssistControlStyle;
    return SaveChange(changed);
}

ProgressUpdateResult StageProgressSystem::SaveChange(bool changed) const
{
    if (!changed) {
        return {};
    }
    if (!mIsPersistenceEnabled) {
        return {true, ProgressSaveStatus::Disabled};
    }
    return {true, Save() ? ProgressSaveStatus::Succeeded : ProgressSaveStatus::Failed};
}
