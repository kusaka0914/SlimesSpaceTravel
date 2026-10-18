#include "system/StageFlowController.h"

#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
#include "system/GameWorld.h"
#include "system/PauseMenuController.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"

StageFlowController::StageFlowController()
    : mCurrentStageYamlPath("../assets/data/stage/house.yaml")
{
}

bool StageFlowController::ReloadCurrentStage(ActorLoadSystem& actorLoader, PhysicsSystem& physicsSystem,
                                             AudioSystem& audioSystem, StagePhysicsReloadMode physicsReloadMode)
{
    if (!actorLoader.LoadData()) {
        return false;
    }
    if (physicsReloadMode == StagePhysicsReloadMode::Rebuild) {
        physicsSystem.Initialize();
    }
    audioSystem.TryChangeBGM();
    return true;
}

void StageFlowController::ChangeStage(GameWorld& world, int stageNum)
{
    if (!world.ChangeStage(stageNum)) {
        return;
    }

    mCurrentStageYamlPath = "../assets/data/stage/stage" + std::to_string(stageNum) + ".yaml";
}

void StageFlowController::ReturnToBase(PauseMenuController& pauseMenu, const GameWorld& world, SceneSystem& sceneSystem)
{
    pauseMenu.Close();

    if (world.IsInBase()) {
        return;
    }

    sceneSystem.RequestStageChange(0);
}
