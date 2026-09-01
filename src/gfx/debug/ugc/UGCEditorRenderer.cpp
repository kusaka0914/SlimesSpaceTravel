#include "gfx/debug/ugc/UGCEditorRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "gfx/UIRenderer.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageSelectionOverlay.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

UGCEditorRenderer::UGCEditorRenderer(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageActorYamlWriter& stageActorYamlWriter,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController,
    std::function<bool()> isAdjustingUGCUI)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mStageActorYamlWriter(stageActorYamlWriter),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mIsAdjustingUGCUI(std::move(isAdjustingUGCUI)),
      mElementRegistrar(mContext),
      mSwitchConnectionController(
          mContext,
          mStageAddActorPanel,
          mSelectionController,
          mToolState,
          mConnectionState),
      mViewController(mContext, mEditorTutorial),
      mInteractionController(
          mContext,
          mStageAddActorPanel,
          mStageActorYamlWriter,
          mSelectionController,
          mEditCommandController,
          mEditorTutorial,
          mToolState,
          mDragState,
          mConnectionState,
          mViewController),
      mPreviewRenderer(
          mContext,
          mToolState,
          mIsAdjustingUGCUI),
      mTutorialOverlayRenderer(
          mContext,
          mEditorTutorial),
      mSceneOverlayRenderer(
          mContext,
          mStageAddActorPanel,
          mSelectionController,
          mInteractionController,
          mSwitchConnectionController,
          mToolState,
          mConnectionState),
      mWorkPanel(
          mContext,
          mMenuState,
          mToolState,
          [this]() {
              mSelectionController.Clear();
              ActivateSelectionMode();
              mConnectionState.Cancel();
              mDragState.isDragging = false;
              mDragState.isMovingPlatformDestination = false;
              mEditCommandController.ClearHistory();
              mToolState.editLayer = 0;
              mStageAddActorPanel.SetUGCEditLayer(mToolState.editLayer);
              mSelectionController.SetUGCEditLayer(mToolState.editLayer);
              if (mContext.game) {
                  mContext.game->SetUGCPreviewEditLayer(mToolState.editLayer);
                  mContext.game->ReloadCurrentStage();
              }
          }),
      mModelThumbnailRenderer(
          std::make_unique<EditorModelThumbnailRenderer>(mContext.game)),
      mDebugOverlayRenderer(
          mContext,
          mStageAddActorPanel,
          mInteractionController,
          mPreviewRenderer,
          mSceneOverlayRenderer,
          mToolState,
          mMenuState,
          mWorkPanel,
          mModelThumbnailRenderer.get(),
          mIsAdjustingUGCUI),
      mChromeRenderer(
          mContext,
          mStageAddActorPanel,
          mInteractionController,
          mEditorTutorial,
          mTutorialOverlayRenderer,
          mToolState,
          mMenuState,
          mWorkPanel,
          mModelThumbnailRenderer.get())
{
    mStageAddActorPanel.SetPlacementCompletedCallback([this]() {
        if (mToolState.activePresetKind) {
            mEditorTutorial.RecordPlacement(
                *mToolState.activePresetKind,
                mToolState.platformFootprintSideLength);
        }
    });
    mStageAddActorPanel.SetUGCEditLayer(mToolState.editLayer);
    mSelectionController.SetUGCEditLayer(mToolState.editLayer);
    if (mContext.game) {
        mContext.game->SetUGCPreviewEditLayer(mToolState.editLayer);
    }
}





void UGCEditorRenderer::OpenMenu()
{
    mMenuState.RequestMenuOpen();
}


void UGCEditorRenderer::HandleUndo()
{
    mInteractionController.HandleUndo();
}

void UGCEditorRenderer::HandleRedo()
{
    mInteractionController.HandleRedo();
}

void UGCEditorRenderer::ToggleEraser()
{
    mInteractionController.ToggleEraser();
}

void UGCEditorRenderer::ActivateSelectionMode()
{
    mInteractionController.ActivateSelectionMode();
}

void UGCEditorRenderer::AdjustZoom(float distanceMultiplier)
{
    mInteractionController.AdjustZoom(distanceMultiplier);
}

void UGCEditorRenderer::ChangeLayer(int layerDelta)
{
    mInteractionController.ChangeLayer(layerDelta);
}

void UGCEditorRenderer::MoveSelectionOnGrid(int gridX, int gridZ)
{
    mInteractionController.MoveSelectionOnGrid(gridX, gridZ);
}

void UGCEditorRenderer::HandleTutorialReturnedFromPlaytest()
{
    mEditorTutorial.RecordReturnedFromPlaytest();
}




void UGCEditorRenderer::RegisterUIEditorElements()
{
    mElementRegistrar.RegisterElements();
}


void UGCEditorRenderer::DrawDebugEditorOverlay()
{
    mDebugOverlayRenderer.Draw();
}

bool UGCEditorRenderer::IsWorkManagementOpen() const
{
    return mWorkPanel.IsManagementOpen();
}

void UGCEditorRenderer::DrawWorkManagement()
{
    mWorkPanel.DrawManagement();
}



void UGCEditorRenderer::StartVerification()
{
    mWorkPanel.StartVerification();
}

void UGCEditorRenderer::DrawWorkBrowser()
{
    mWorkPanel.DrawBrowser();
}

bool UGCEditorRenderer::CompleteVerification(
    const std::string& workFileName)
{
    return mWorkPanel.CompleteVerification(workFileName);
}

void UGCEditorRenderer::Draw(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const bool isTutorialStage =
        mContext.game &&
        mContext.game->GetIsUGCEditorTutorialActive();
    if (isTutorialStage && !mEditorTutorial.IsActive()) {
        mEditorTutorial.Start();
    } else if (!isTutorialStage && mEditorTutorial.IsActive()) {
        mEditorTutorial.Stop();
    }

    if (mChromeRenderer.DrawControls()) {
        return;
    }

    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const ImVec2 gameViewportMin(
        mainViewport->WorkPos.x,
        mainViewport->WorkPos.y);
    const ImVec2 gameViewportMax(
        mainViewport->WorkPos.x + mainViewport->WorkSize.x,
        mainViewport->WorkPos.y + mainViewport->WorkSize.y);
    mPreviewRenderer.DrawGameViewport(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight,
        gameViewportMin,
        gameViewportMax);
    const bool isWorkManagementOpen = IsWorkManagementOpen();
    if (!isWorkManagementOpen) {
        mSceneOverlayRenderer.DrawBackgroundGuides();
        mPreviewRenderer.DrawPreviewOverlay();
    }

    mInteractionController.UpdateSceneInteraction();
    if (!isWorkManagementOpen) {
        DrawStageSelectionOverlay(mSelectionController);
        mSceneOverlayRenderer.DrawSelectionOverlays();
    }
    mTutorialOverlayRenderer.Draw();
    if (!isWorkManagementOpen && mContext.uiRenderer) {
        mContext.uiRenderer->DrawUGCForegroundCustomUI(
            gameViewportMin,
            ImVec2(
                gameViewportMax.x - gameViewportMin.x,
                gameViewportMax.y - gameViewportMin.y));
    }
    DrawWorkManagement();
}


























