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

private:
    std::string mSavePath;
    std::set<int> mClearedStages;
};
