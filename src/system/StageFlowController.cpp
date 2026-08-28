#include "system/StageFlowController.h"

#include "Game.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
#include "system/GameWorld.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"

StageFlowController::StageFlowController()
    : mCurrentStageYamlPath("../assets/data/stage/house.yaml")
{
}

void StageFlowController::LoadData(Game& game, bool isLoadPlayer)
{
    game.RemoveAllActor();
    game.GetActorLoadSystem()->LoadData(isLoadPlayer);
}

void StageFlowController::ReloadCurrentStage(
    Game& game,
    StagePhysicsReloadMode physicsReloadMode)
{
    LoadData(game, true);
    if (physicsReloadMode == StagePhysicsReloadMode::Rebuild) {
        game.GetPhysicsSystem()->Initialize();
    }
    game.GetAudioSystem()->TryChangeBGM();
}

void StageFlowController::ChangeStage(GameWorld& world, int stageNum)
{
    if (!world.ChangeStage(stageNum)) {
        return;
    }

    mCurrentStageYamlPath = "../assets/data/stage/stage" + std::to_string(stageNum) + ".yaml";
}

void StageFlowController::ReturnToBase(Game& game)
{
    game.ClosePauseMenu();

    if (game.IsInBase()) {
        return;
    }

    game.GetSceneSystem()->RequestStageChange(0);
}
