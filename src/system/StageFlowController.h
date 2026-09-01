#pragma once

#include <string>

class Game;
class GameWorld;
enum class StagePhysicsReloadMode;

class StageFlowController {
public:
    StageFlowController();

    bool LoadData(Game& game);
    bool ReloadCurrentStage(
        Game& game,
        StagePhysicsReloadMode physicsReloadMode);
    void ChangeStage(GameWorld& world, int stageNum);
    void ReturnToBase(Game& game);

    void SetCurrentStageYamlPath(const std::string& yamlPath) { mCurrentStageYamlPath = yamlPath; }
    const std::string& GetCurrentStageYamlPath() const { return mCurrentStageYamlPath; }

private:
    std::string mCurrentStageYamlPath;
};
