#include "editor/ugc/UGCEditorRenderer.h"

#include "app/Game.h"
#include "editor/stage/history/StageEditCommandController.h"
#include "editor/stage/panels/StageAddActorPanel.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "editor/stage/selection/StageSelectionOverlay.h"
#include "gfx/UIRenderer.h"
#include "imgui.h"

#include <utility>

UGCEditorRenderer::UGCEditorRenderer(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
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
      mSwitchConnectionController(mContext, mStageAddActorPanel, mSelectionController, mToolState, mConnectionState),
      mViewController(mContext, mEditorTutorial),
      mStageSession(mContext),
      mInteractionController(mStageSession, mStageAddActorPanel, mStageActorYamlWriter, mSelectionController,
                             mEditCommandController, mEditorTutorial, mToolState, mDragState, mConnectionState,
                             mViewController),
      mPreviewRenderer(mContext, mToolState, mIsAdjustingUGCUI),
      mTutorialOverlayRenderer(mContext, mEditorTutorial),
      mSceneOverlayRenderer(mContext, mStageAddActorPanel, mSelectionController, mInteractionController,
                            mSwitchConnectionController, mToolState, mConnectionState),
      mWorkPanel(mContext, mMenuState, mToolState, [this]() { ReloadSelectedWork(); }),
      mModelThumbnailRenderer(std::make_unique<EditorModelThumbnailRenderer>(mContext.game)),
      mDebugOverlayRenderer(mContext, mStageAddActorPanel, mInteractionController, mPreviewRenderer,
                            mSceneOverlayRenderer, mToolState, mMenuState, mWorkPanel, mModelThumbnailRenderer.get(),
                            mIsAdjustingUGCUI),
      mChromeRenderer(mContext, mStageAddActorPanel, mInteractionController, mEditorTutorial, mTutorialOverlayRenderer,
                      mToolState, mMenuState, mWorkPanel, mModelThumbnailRenderer.get())
{
    mStageAddActorPanel.SetPlacementCompletedCallback([this]() {
        if (mToolState.activePresetKind) {
            mEditorTutorial.RecordPlacement(*mToolState.activePresetKind, mToolState.platformFootprintSideLength);
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

void UGCEditorRenderer::DrawWorkBrowser()
{
    mWorkPanel.DrawBrowser();
}

bool UGCEditorRenderer::CompleteVerification(const std::string& workFileName)
{
    return mWorkPanel.CompleteVerification(workFileName);
}

void UGCEditorRenderer::Draw(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight)
{
    const bool isTutorialStage = mContext.game && mContext.game->GetIsUGCEditorTutorialActive();
    if (isTutorialStage && !mEditorTutorial.IsActive()) {
        mEditorTutorial.Start();
    } else if (!isTutorialStage && mEditorTutorial.IsActive()) {
        mEditorTutorial.Stop();
    }

    if (mChromeRenderer.DrawControls()) {
        return;
    }

    const std::string& saveError = mStageActorYamlWriter.GetSaveError().empty() ? mEditCommandController.GetLastError()
                                                                                : mStageActorYamlWriter.GetSaveError();
    if (!saveError.empty()) {
        ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_Always);
        if (ImGui::Begin("保存・編集エラー", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("%s", saveError.c_str());
            if (!mStageActorYamlWriter.GetSaveError().empty() && ImGui::Button("保存を再試行")) {
                mToolState.statusMessage = mStageActorYamlWriter.RetryFailedSaves()
                                               ? "編集内容を保存しました"
                                               : "保存できませんでした。保存先を確認してください";
            }
        }
        ImGui::End();
    }

    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const ImVec2 gameViewportMin(mainViewport->WorkPos.x, mainViewport->WorkPos.y);
    const ImVec2 gameViewportMax(mainViewport->WorkPos.x + mainViewport->WorkSize.x,
                                 mainViewport->WorkPos.y + mainViewport->WorkSize.y);
    mPreviewRenderer.DrawGameViewport(gameViewTexture, gameViewWidth, gameViewHeight, gameViewportMin, gameViewportMax);
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
            gameViewportMin, ImVec2(gameViewportMax.x - gameViewportMin.x, gameViewportMax.y - gameViewportMin.y));
    }
    DrawWorkManagement();
}

void UGCEditorRenderer::ReloadSelectedWork()
{
    if (mContext.game && !mContext.game->ReloadCurrentStage()) {
        mToolState.statusMessage = "ステージの再読み込みに失敗しました";
        return;
    }

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
    }
}
