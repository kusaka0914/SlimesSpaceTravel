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

private:
    std::string mSavePath;
    std::set<int> mClearedStages;
};
