#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "imgui.h"

#include <utility>
#include <vector>

StageAddActorPanel::StageAddActorPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mCreateService(context),
      mUGCPlatformCellService(context),
      mPlacementResolver(context),
      mUGCPlatformEditController(
          context,
          mUGCPlatformCellService,
          mPlacementResolver),
      mPlacementController(
          context,
          mCreateService,
          mUGCPlatformCellService,
          mPlacementResolver),
      mUGCPresetController(
          context,
          mCreateService,
          mUGCPlatformCellService,
          mPlacementController),
      mJewelItemCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mHazardActorCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mBoatArrivalPointCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mEnemyCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mNPCCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mTutorialTriggerCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mStageObjectCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mPlanetCreationForm(
          mCreateService),
      mPlatformCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mCrystalCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mBoatPartsCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mBoatCreationForm(
          context,
          mCreateService,
          mPlacementController),
      mStarCreationForm(
          context,
          mCreateService,
          mPlacementController)
{
}

void StageAddActorPanel::SetSelectionController(
    StageSelectionController* selectionController)
{
    mPlacementController.SetSelectionController(selectionController);
    mUGCPlatformEditController.SetSelectionController(selectionController);
    mUGCPresetController.SetSelectionController(selectionController);
}

void StageAddActorPanel::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mUGCPlatformEditController.SetPushUndoCallback(pushUndoCallback);
    mUGCPresetController.SetPushUndoCallback(pushUndoCallback);
    mPlacementController.SetPushUndoCallback(
        std::move(pushUndoCallback));
}

bool StageAddActorPanel::ActivateUGCPreset(UGCPresetKind presetKind)
{
    return mUGCPresetController.ActivatePreset(presetKind);
}

bool StageAddActorPanel::TryEraseUGCPlatformCell()
{
    return mUGCPlatformEditController.TryEraseCell();
}

void StageAddActorPanel::EndUGCEraseGesture()
{
    mUGCPlatformEditController.EndEraseGesture();
}

bool StageAddActorPanel::TryTranslateUGCPlatformCells(
    const StageActorRef& actorRef,
    const glm::vec3& worldDelta)
{
    return mUGCPlatformEditController.TryTranslateCells(
        actorRef,
        worldDelta);
}

bool StageAddActorPanel::TryTranslateUGCPlatformCells(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mUGCPlatformEditController.TryTranslateCells(
        actorRefs,
        worldDelta);
}

bool StageAddActorPanel::TryTranslateUGCMovingPlatformDestinations(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mUGCPlatformEditController.TryTranslateMovingPlatformDestinations(
        actorRefs,
        worldDelta);
}

bool StageAddActorPanel::TrySaveUGCMovingPlatformDestinationTranslation(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mUGCPlatformEditController
        .TrySaveMovingPlatformDestinationTranslation(actorRefs, worldDelta);
}

bool StageAddActorPanel::BeginDuplicatePlacement(
    const StageActorRef& sourceRef)
{
    return mPlacementController.BeginDuplicatePlacement(sourceRef);
}

void StageAddActorPanel::UpdatePlacement()
{
    mPlacementController.UpdatePlacement();
}

void StageAddActorPanel::CancelPlacement()
{
    mPlacementController.CancelPlacement();
}

void StageAddActorPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    mBoatArrivalPointCreationForm.Draw();
    mJewelItemCreationForm.Draw();
    mHazardActorCreationForm.Draw();

    if (mPlacementController.IsPlacementActive()) {
        ImGui::SeparatorText("連続配置中");
        ImGui::Text(
            "配置対象: %s",
            mPlacementController.GetPlacementDisplayName().c_str());
        ImGui::TextWrapped("ゲーム画面をクリックするたびに追加します。");
        if (ImGui::Button("追加解除") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelPlacement();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("ESCでも解除");
        if (!mPlacementController.GetPlacementStatus().empty()) {
            ImGui::TextWrapped(
                "%s",
                mPlacementController.GetPlacementStatus().c_str());
        }
        ImGui::Separator();
    }

    mStageObjectCreationForm.Draw();

    mPlanetCreationForm.Draw();

    if (!mEnemyCreationForm.Draw()) {
        return;
    }

    mPlatformCreationForm.Draw();

    mCrystalCreationForm.Draw();

    mNPCCreationForm.Draw();

    mTutorialTriggerCreationForm.Draw();

    mBoatPartsCreationForm.Draw();

    mBoatCreationForm.Draw();

    mStarCreationForm.Draw();
}
