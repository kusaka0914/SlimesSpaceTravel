#include "editor/core/DebugUIRenderer.h"

#include "app/Game.h"
#include "gfx/UIRenderer.h"
#include "editor/core/DebugEditorLayout.h"
#include "editor/session/DebugEditorSessionCoordinator.h"
#include "editor/ugc/UGCEditorRenderer.h"

#include <string>

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{.game = game, .uiRenderer = uiRenderer, .assetCatalog = &mAssetCatalog},
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
      mStarCollectionPanel(mContext, mBuildRestartPanel),
      mSequenceEditorPanel(mCinematicCameraPanel, mSequencePanel, mEndingRollPanel, mStorybookPanel,
                           mStarCollectionPanel),
      mTutorialPanel(mContext),
      mAssetBrowserPanel(mContext),
      mStageAddActorPanel(mContext),
      mStagePlanetPanel(mContext,
                        [this]() {
                            return mStageActorYamlWriter.SaveEditorAuthoredTransforms(
                                [this](const std::string& before) { mEditCommandController.RecordSavedEdit(before); });
                        }),
      mStageActorYamlWriter(mContext),
      mSelectionController(
          [game]() {
              if (!game) {
                  return StageSelectionScene{};
              }
              return StageSelectionScene{
                  game->GetCurrentStage(),    game->GetPhysicsSystem(),         game->GetCameraSystem(),
                  game->GetWindow(),          game->GetCurrentStageYamlPath(),  game->GetIsUGCMode(),
                  game->GetIsPlayer2Joined(), game->GetIsUGCOrthographicView(), game->GetUGCOrthographicHalfHeight(),
                  game->GetUGCGridSize()};
          },
          mContext.gameViewport, mContext.planetMoveMode),
      mStagePlacementPanel(
          mContext, mSelectionController, mStageActorYamlWriter,
          [this](const std::string& previousYamlText) { mEditCommandController.RecordSavedEdit(previousYamlText); }),
      mEditCommandController(mContext, mSelectionController),
      mStageDeleteActorPanel(mContext, mEditCommandController),
      mStageEditorPanel(mContext, mStageAddActorPanel, mStagePlanetPanel, mStagePlacementPanel, mStageDeleteActorPanel,
                        mStageActorYamlWriter, mSelectionController, mEditCommandController),
      mGizmoController(mContext, mSelectionController,
                       [this]() {
                           return mEditCommandController.ExecuteEdit(
                               [this]() { return mStagePlanetPanel.SaveEditorAuthoredTransforms(); });
                       }),
      mGameViewportRenderer(mContext, mSelectionController, mGizmoController, mBuildRestartPanel),
      mWorkspaceRenderer(DebugEditorWorkspaceDependencies{
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
    mStageAddActorPanel.SetSelectionController(&mSelectionController);
    mStageAddActorPanel.SetEditExecutor([this](const StageEditOperation& operation, StageEditGrouping grouping) {
        return mEditCommandController.ExecuteEdit(operation, grouping);
    });
    mSessionCoordinator = std::make_unique<DebugEditorSessionCoordinator>(mContext, mStagePlanetPanel,
                                                                          mStageEditorPanel, mSelectionController);
    mUGCEditorRenderer = std::make_unique<UGCEditorRenderer>(
        mContext, mStageAddActorPanel, mStageActorYamlWriter, mSelectionController, mEditCommandController,
        [this]() { return mWorkspaceRenderer.IsUserInterfaceSectionActive(); });
}

DebugUIRenderer::~DebugUIRenderer() = default;

void DebugUIRenderer::DrawUGCEditor(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight)
{
    mUGCEditorRenderer->Draw(gameViewTexture, gameViewWidth, gameViewHeight);
}

void DebugUIRenderer::DrawUGCWorkBrowser()
{
    mUGCEditorRenderer->DrawWorkBrowser();
}

bool DebugUIRenderer::CompleteUGCVerification(const std::string& workFileName)
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

void DebugUIRenderer::NotifyStageReloaded()
{
    mEditCommandController.SynchronizeStage();
    mStageActorYamlWriter.ClearFailedSaves();
}

bool DebugUIRenderer::SaveEditorSession(const std::string& filePath, std::string& outErrorMessage)
{
    return mSessionCoordinator->Save(filePath, mWorkspaceRenderer.CaptureShellState(), outErrorMessage);
}

bool DebugUIRenderer::RestoreEditorSession(const std::string& filePath, std::string& outErrorMessage)
{
    DebugEditorShellSessionState shellState;
    if (!mSessionCoordinator->Restore(filePath, shellState, outErrorMessage)) {
        return false;
    }

    mWorkspaceRenderer.ApplyShellState(shellState);
    return true;
}

void DebugUIRenderer::SetBuildRestartStatus(const std::string& message, bool isError)
{
    mBuildRestartPanel.SetStatus(message, isError);
}

void DebugUIRenderer::Draw(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight)
{
    mWorkspaceRenderer.Draw(gameViewTexture, gameViewWidth, gameViewHeight);

    if (!mContext.game || !mContext.game->GetIsUGCMode()) {
        return;
    }
    mUGCEditorRenderer->RegisterUIEditorElements();
    mUGCEditorRenderer->DrawDebugEditorOverlay();
}
