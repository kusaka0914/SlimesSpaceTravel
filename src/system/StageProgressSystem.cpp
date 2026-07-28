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
    if (!clearedStages || !clearedStages.IsSequence()) {
        return true;
    }

    for (const YAML::Node& stageNode : clearedStages) {
        const int stageNum = stageNode.as<int>();
        if (stageNum >= 0) {
            mClearedStages.insert(stageNum);
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
