#include "system/StageProgressSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <yaml-cpp/yaml.h>

StageProgressSystem::StageProgressSystem(std::string savePath)
    : mSavePath(std::move(savePath))
{
}

bool StageProgressSystem::Load()
{
    mClearedStages.clear();
    mShownConversationIds.clear();
    mHasCompletedEndingRoll = false;
    mHasSelectedPlayerControlStyle = false;
    mIsAssistControlStyleSelected = false;

    YAML::Node root;
    try {
        root = YAML::LoadFile(mSavePath);
    } catch (const YAML::BadFile&) {
        return true;
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load stage progress: "
                  << exception.what() << '\n';
        return false;
    }

    const YAML::Node clearedStages = root["clearedStages"];
    if (clearedStages && clearedStages.IsSequence()) {
        for (const YAML::Node& stageNode : clearedStages) {
            const int stageNum = stageNode.as<int>();
            if (stageNum >= 0) {
                mClearedStages.insert(stageNum);
            }
        }
    }

    const YAML::Node shownConversations = root["shownConversations"];
    if (shownConversations && shownConversations.IsSequence()) {
        for (const YAML::Node& conversationNode : shownConversations) {
            const std::string conversationId =
                conversationNode.as<std::string>();
            if (!conversationId.empty()) {
                mShownConversationIds.insert(conversationId);
            }
        }
    }

    // Migrate progress written by builds that temporarily stored this as a
    // conversation ID.  The next save writes the dedicated property below.
    const YAML::Node endingRollCompleted = root["endingRollCompleted"];
    mHasCompletedEndingRoll =
        endingRollCompleted && endingRollCompleted.IsScalar()
            ? endingRollCompleted.as<bool>()
            : mShownConversationIds.contains("cinematic:ending_roll_completed");

    const YAML::Node playerControlStyle = root["playerControlStyle"];
    if (playerControlStyle && playerControlStyle.IsScalar()) {
        const std::string style = playerControlStyle.as<std::string>();
        if (style == "assist" || style == "standard") {
            mHasSelectedPlayerControlStyle = true;
            mIsAssistControlStyleSelected = style == "assist";
        }
    }

    return true;
}

bool StageProgressSystem::Save() const
{
    const std::filesystem::path savePath(mSavePath);
    std::error_code error;
    if (savePath.has_parent_path()) {
        std::filesystem::create_directories(savePath.parent_path(), error);
        if (error) {
            std::cerr << "Failed to create stage progress directory: "
                      << error.message() << '\n';
            return false;
        }
    }

    YAML::Node root;
    root["clearedStages"] = YAML::Node(YAML::NodeType::Sequence);
    for (int stageNum : mClearedStages) {
        root["clearedStages"].push_back(stageNum);
    }
    root["shownConversations"] =
        YAML::Node(YAML::NodeType::Sequence);
    for (const std::string& conversationId :
         mShownConversationIds) {
        root["shownConversations"].push_back(conversationId);
    }
    root["endingRollCompleted"] = mHasCompletedEndingRoll;
    if (mHasSelectedPlayerControlStyle) {
        root["playerControlStyle"] =
            mIsAssistControlStyleSelected ? "assist" : "standard";
    }

    std::ofstream file(mSavePath);
    if (!file.is_open()) {
        std::cerr << "Failed to save stage progress: " << mSavePath << '\n';
        return false;
    }

    file << root;
    return true;
}

bool StageProgressSystem::IsStageCleared(int stageNum) const
{
    return stageNum >= 0 && mClearedStages.contains(stageNum);
}

bool StageProgressSystem::MarkStageCleared(int stageNum)
{
    return SetStageCleared(stageNum, true);
}

bool StageProgressSystem::SetStageCleared(
    int stageNum,
    bool isCleared)
{
    if (stageNum < 0) {
        return false;
    }

    bool changed = false;
    if (isCleared) {
        changed = mClearedStages.insert(stageNum).second;
    } else {
        changed = mClearedStages.erase(stageNum) > 0;
    }

    if (changed) {
        Save();
    }
    return changed;
}

bool StageProgressSystem::HasShownConversation(
    const std::string& conversationId) const
{
    return !conversationId.empty() &&
           mShownConversationIds.contains(conversationId);
}

bool StageProgressSystem::MarkConversationShown(
    const std::string& conversationId)
{
    if (conversationId.empty()) {
        return false;
    }

    const bool changed =
        mShownConversationIds.insert(conversationId).second;
    if (changed) {
        Save();
    }
    return changed;
}

bool StageProgressSystem::SetEndingRollCompleted(bool completed)
{
    if (mHasCompletedEndingRoll == completed) {
        return false;
    }

    mHasCompletedEndingRoll = completed;
    Save();
    return true;
}

bool StageProgressSystem::SetSelectedPlayerControlStyle(
    bool isAssistControlStyle)
{
    const bool changed = !mHasSelectedPlayerControlStyle ||
                         mIsAssistControlStyleSelected != isAssistControlStyle;
    mHasSelectedPlayerControlStyle = true;
    mIsAssistControlStyleSelected = isAssistControlStyle;
    if (changed) {
        Save();
    }
    return changed;
}
