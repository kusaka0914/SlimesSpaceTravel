#pragma once

#include <string>

class Game;
class GameWorld;

class StageFlowController {
public:
    StageFlowController();

    void LoadData(Game& game, bool isLoadPlayer);
    void ReloadCurrentStage(Game& game, bool rebuildPhysics = true);
    void ChangeStage(GameWorld& world, int stageNum);
    void ReturnToBase(Game& game);

    void SetCurrentStageYamlPath(const std::string& yamlPath) { mCurrentStageYamlPath = yamlPath; }
    const std::string& GetCurrentStageYamlPath() const { return mCurrentStageYamlPath; }

private:
    std::string mCurrentStageYamlPath;
};
