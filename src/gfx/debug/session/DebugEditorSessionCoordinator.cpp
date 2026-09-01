#include "gfx/debug/session/DebugEditorSessionCoordinator.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/panels/StageEditorPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/session/EditorSessionState.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "system/CameraSystem.h"

#include <glm/glm.hpp>
#include <unordered_set>
#include <vector>

DebugEditorSessionCoordinator::DebugEditorSessionCoordinator(
    DebugEditorContext& context,
    StagePlanetPanel& stagePlanetPanel,
    StageEditorPanel& stageEditorPanel,
    StageSelectionController& selectionController)
    : mContext(context),
      mStagePlanetPanel(stagePlanetPanel),
      mStageEditorPanel(stageEditorPanel),
      mSelectionController(selectionController)
{
}

bool DebugEditorSessionCoordinator::Save(
    const std::string& filePath,
    const DebugEditorShellSessionState& shellState,
    std::string& outErrorMessage)
{
    // 実行時の移動は保存せず、編集操作で変更したTransformだけを保存する。
    mStagePlanetPanel.SaveEditorAuthoredTransforms();

    EditorSessionState sessionState;
    if (mContext.game) {
        sessionState.stageNumber = mContext.game->GetCurrentStageNum();
        sessionState.stageYamlPath = mContext.game->GetCurrentStageYamlPath();
        sessionState.activeSectionIndex = shellState.activeSectionIndex;
        sessionState.sequenceEditorMenuIndex =
            shellState.sequenceEditorMenuIndex;
        sessionState.stageEditorMenuIndex =
            mStageEditorPanel.GetSelectedMenu();
        sessionState.isEditorShowing =
            mContext.game->GetIsDebugEditorShowing();
        sessionState.isSceneView =
            mContext.game->GetIsFreeCameraMode();
        sessionState.rightPanelWidth = mContext.layout.rightPanelWidth;
        sessionState.assetBrowserHeight =
            mContext.layout.assetBrowserHeight;

        if (CameraSystem* cameraSystem =
                mContext.game->GetCameraSystem()) {
            sessionState.sceneCameraPose =
                cameraSystem->GetDebugCameraPose();
        }

        if (Player* player = mContext.game->GetMainPlayer()) {
            sessionState.hasPlayerDebugPose = true;
            sessionState.playerPosition = player->GetPos();
            sessionState.playerUp = player->GetUpVec();
            sessionState.playerOrientation = player->GetOrientation();
            sessionState.playerPlanetIndex =
                player->GetCurrentPlanetNum();
        }

        const std::unordered_set<std::string>& selectedKeys =
            mSelectionController.GetSelectedKeys();
        sessionState.selectedActorKeys.assign(
            selectedKeys.begin(),
            selectedKeys.end());
    }

    return EditorSessionRepository::Save(
        filePath,
        sessionState,
        outErrorMessage);
}

bool DebugEditorSessionCoordinator::Restore(
    const std::string& filePath,
    DebugEditorShellSessionState& outShellState,
    std::string& outErrorMessage)
{
    EditorSessionState sessionState;
    if (!EditorSessionRepository::Load(
            filePath,
            sessionState,
            outErrorMessage)) {
        return false;
    }

    if (!mContext.game) {
        outErrorMessage =
            "The game is not available while restoring the editor session.";
        return false;
    }

    if (!mContext.game->RestoreDebugEditorStage(
            sessionState.stageNumber,
            sessionState.stageYamlPath)) {
        outErrorMessage = "Failed to restore the edited stage: " +
                          sessionState.stageYamlPath;
        return false;
    }

    outShellState.activeSectionIndex = sessionState.activeSectionIndex;
    outShellState.sequenceEditorMenuIndex =
        sessionState.sequenceEditorMenuIndex;
    mStageEditorPanel.SetSelectedMenu(
        sessionState.stageEditorMenuIndex);
    mContext.layout.rightPanelWidth = sessionState.rightPanelWidth;
    mContext.layout.assetBrowserHeight = sessionState.assetBrowserHeight;

    mContext.game->SetDebugEditorShowing(sessionState.isEditorShowing);
    mContext.game->SetFreeCameraMode(sessionState.isSceneView);
    if (CameraSystem* cameraSystem = mContext.game->GetCameraSystem()) {
        cameraSystem->SetDebugCameraPose(sessionState.sceneCameraPose);
    }

    if (sessionState.hasPlayerDebugPose) {
        Player* player = mContext.game->GetMainPlayer();
        Stage* currentStage = mContext.game->GetCurrentStage();
        if (player && currentStage) {
            const std::vector<Planet*>& planets =
                currentStage->GetPlanets();
            const int planetIndex = sessionState.playerPlanetIndex;
            if (planetIndex >= 0 &&
                planetIndex < static_cast<int>(planets.size())) {
                player->SetCurrentPlanet(planets[planetIndex]);
                player->SetCurrentPlanetNum(planetIndex);
            }
            player->SetPos(sessionState.playerPosition);
            player->SetUpVec(sessionState.playerUp);
            player->SetOrientation(sessionState.playerOrientation);
            player->SetVelocity(glm::vec3(0.0f));
        }
    }

    std::unordered_set<std::string> selectedKeys(
        sessionState.selectedActorKeys.begin(),
        sessionState.selectedActorKeys.end());
    if (selectedKeys.empty()) {
        mSelectionController.Clear();
        return true;
    }

    if (selectedKeys.size() != 1) {
        mSelectionController.SetSelectedKeys(selectedKeys);
        mSelectionController.ConsumeRequestOpenPlacement();
        return true;
    }

    const std::vector<StageActorInstance> actorInstances =
        StageActorQuery::CollectAllActorInstances(
            mContext.game->GetCurrentStage());
    for (const StageActorInstance& actorInstance : actorInstances) {
        if (actorInstance.actor &&
            selectedKeys.contains(
                StageActorQuery::MakeKey(actorInstance.ref))) {
            mSelectionController.SetSingleSelection(
                actorInstance.actor,
                actorInstance.ref);
            mSelectionController.ConsumeRequestOpenPlacement();
            return true;
        }
    }

    mSelectionController.Clear();
    return true;
}
