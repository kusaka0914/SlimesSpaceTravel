#include "DebugUIRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Star.h"
#include "actor/Platform.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/session/DebugEditorSessionCoordinator.h"
#include "gfx/debug/ugc/UGCEditorRenderer.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer, &mAssetCatalog},
      mLayoutController(mContext),
      mBuildRestartPanel(mContext),
      mPerformancePanel(mContext),
      mBasicInfoPanel(mPerformancePanel),
      mCameraPanel(mContext),
      mCinematicCameraPanel(mContext),
      mUIPanel(mContext),
      mParameterPanel(mContext, mCameraPanel),
      mParticleEffectPanel(mContext),
      mSequencePanel(mContext),
      mEndingRollPanel(mContext),
      mStorybookPanel(mContext),
      mStarCollectionPanel(
          mContext,
          mBuildRestartPanel),
      mSequenceEditorPanel(
          mCinematicCameraPanel,
          mSequencePanel,
          mEndingRollPanel,
          mStorybookPanel,
          mStarCollectionPanel),
      mTutorialPanel(mContext),
      mAssetBrowserPanel(mContext),
      mStageAddActorPanel(mContext),
      mStagePlanetPanel(mContext),
      mStageActorYamlWriter(mContext),
      mSelectionController(mContext),
      mStagePlacementPanel(
          mContext,
          mSelectionController,
          mStageActorYamlWriter,
          [this]() { mEditCommandController.PushUndo(); }),
      mEditCommandController(mContext, mSelectionController),
      mStageDeleteActorPanel(mContext, mEditCommandController),
      mStageEditorPanel(
          mContext,
          mStageAddActorPanel,
          mStagePlanetPanel,
          mStagePlacementPanel,
          mStageDeleteActorPanel,
          mStageActorYamlWriter,
          mSelectionController,
          [this]() { return mEditCommandController.RestoreUndo(); },
          [this]() { return mEditCommandController.RestoreRedo(); }),
      mGizmoController(
          mContext, mSelectionController, [this]() { mEditCommandController.PushUndo(); },
          [this]() {
              mStagePlanetPanel.SaveEditorAuthoredTransforms();
          }),
      mGameViewportRenderer(
          mContext,
          mSelectionController,
          mGizmoController,
          mBuildRestartPanel),
      mWorkspaceRenderer(
          DebugEditorWorkspaceDependencies{
              .context = mContext,
              .layoutController = mLayoutController,
              .gameViewportRenderer = mGameViewportRenderer,
              .basicInfoPanel = mBasicInfoPanel,
              .uiPanel = mUIPanel,
              .parameterPanel = mParameterPanel,
              .particleEffectPanel = mParticleEffectPanel,
              .sequenceEditorPanel = mSequenceEditorPanel,
              .tutorialPanel = mTutorialPanel,
              .assetBrowserPanel = mAssetBrowserPanel,
              .stageAddActorPanel = mStageAddActorPanel,
              .selectionController = mSelectionController,
              .editCommandController = mEditCommandController,
              .stageEditorPanel = mStageEditorPanel,
              .gizmoController = mGizmoController,
          })
{
    mStagePlanetPanel.SetSaveDependentActorTransformsCallback(
        [this]() { mStageActorYamlWriter.SaveEditorAuthoredTransforms(); });
    mStageAddActorPanel.SetSelectionController(&mSelectionController);
    mStageAddActorPanel.SetPushUndoCallback(
        [this]() { mEditCommandController.PushUndo(); });
    mSessionCoordinator =
        std::make_unique<DebugEditorSessionCoordinator>(
            mContext,
            mStagePlanetPanel,
            mStageEditorPanel,
            mSelectionController);
    mUGCEditorRenderer = std::make_unique<UGCEditorRenderer>(
        mContext,
        mStageAddActorPanel,
        mStageActorYamlWriter,
        mSelectionController,
        mEditCommandController,
        [this]() {
            return mWorkspaceRenderer.IsUserInterfaceSectionActive();
        });
}

DebugUIRenderer::~DebugUIRenderer() = default;

void DebugUIRenderer::DrawUGCEditor(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    mUGCEditorRenderer->Draw(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight);
}

void DebugUIRenderer::DrawUGCWorkBrowser()
{
    mUGCEditorRenderer->DrawWorkBrowser();
}

bool DebugUIRenderer::CompleteUGCVerification(
    const std::string& workFileName)
{
    return mUGCEditorRenderer->CompleteVerification(workFileName);
}

void DebugUIRenderer::HandleUGCUndo()
{
    mUGCEditorRenderer->HandleUndo();
}

void DebugUIRenderer::HandleUGCRedo()
{
    mUGCEditorRenderer->HandleRedo();
}

void DebugUIRenderer::HandleUGCEraserToggle()
{
    mUGCEditorRenderer->ToggleEraser();
}

void DebugUIRenderer::HandleUGCSelectionMode()
{
    mUGCEditorRenderer->ActivateSelectionMode();
}

void DebugUIRenderer::OpenUGCEditorMenu()
{
    mUGCEditorRenderer->OpenMenu();
}

void DebugUIRenderer::HandleUGCZoom(float distanceMultiplier)
{
    mUGCEditorRenderer->AdjustZoom(distanceMultiplier);
}

void DebugUIRenderer::HandleUGCLayerChange(int layerDelta)
{
    mUGCEditorRenderer->ChangeLayer(layerDelta);
}

void DebugUIRenderer::HandleUGCSelectionGridMove(int gridX, int gridZ)
{
    mUGCEditorRenderer->MoveSelectionOnGrid(gridX, gridZ);
}

void DebugUIRenderer::HandleUGCEditorTutorialReturnedFromPlaytest()
{
    mUGCEditorRenderer->HandleTutorialReturnedFromPlaytest();
}

void DebugUIRenderer::ClearStageSelectionForStageReload()
{
    mSelectionController.Clear();
}

bool DebugUIRenderer::SaveEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    return mSessionCoordinator->Save(
        filePath,
        mWorkspaceRenderer.CaptureShellState(),
        outErrorMessage);
}

bool DebugUIRenderer::RestoreEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    DebugEditorShellSessionState shellState;
    if (!mSessionCoordinator->Restore(
            filePath,
            shellState,
            outErrorMessage)) {
        return false;
    }

    mWorkspaceRenderer.ApplyShellState(shellState);
    return true;
}

void DebugUIRenderer::SetBuildRestartStatus(
    const std::string& message,
    bool isError)
{
    mBuildRestartPanel.SetStatus(message, isError);
}





















void DebugUIRenderer::Draw(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    mWorkspaceRenderer.Draw(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight);

    if (!mContext.game || !mContext.game->GetIsUGCMode()) {
        return;
    }
    mUGCEditorRenderer->RegisterUIEditorElements();
    mUGCEditorRenderer->DrawDebugEditorOverlay();
}
