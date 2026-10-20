#include "editor/ugc/UGCEditorInteractionController.h"
#include "editor/ugc/UGCEditorToolState.h"
#include "editor/ugc/UGCEditorTutorial.h"
#include "editor/ugc/UGCEditorViewController.h"
#include "editor/ugc/UGCSelectionDragState.h"

#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "app/Game.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/history/StageEditCommandController.h"
#include "editor/stage/panels/StageAddActorPanel.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "editor/stage/persistence/StagePlanetYamlWriter.h"
#include "editor/stage/persistence/StageYamlRepository.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "editor/stage/ugc/UGCPlatformGrid.h"
#include "editor/ugc/UGCSwitchConnectionState.h"
#include "gameplay/ugc/UGCWorkMetadata.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

UGCEditorInteractionController::UGCEditorInteractionController(
    DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel, StageActorYamlWriter& stageActorYamlWriter,
    StageSelectionController& selectionController, StageEditCommandController& editCommandController,
    UGCEditorTutorial& editorTutorial, UGCEditorToolState& toolState, UGCSelectionDragState& dragState,
    UGCSwitchConnectionState& connectionState, UGCEditorViewController& viewController)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mStageActorYamlWriter(stageActorYamlWriter),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mEditorTutorial(editorTutorial),
      mToolState(toolState),
      mDragState(dragState),
      mConnectionState(connectionState),
      mViewController(viewController)
{
}

void UGCEditorInteractionController::HandleUndo()
{
    const bool wasRestored = mEditCommandController.RestoreUndo();
    mEditorTutorial.RecordUndo(wasRestored);
    mToolState.statusMessage = wasRestored ? "1つ前の状態に戻しました" : "戻せる操作がありません";
}

void UGCEditorInteractionController::HandleRedo()
{
    mToolState.statusMessage =
        mEditCommandController.RestoreRedo() ? "戻した操作をやり直しました" : "やり直せる操作がありません";
}

void UGCEditorInteractionController::ToggleEraser()
{
    if (!mToolState.isEraserMode) {
        const std::optional<UGCPresetKind> presetToRestore =
            mStageAddActorPanel.IsPlacementActive() ? mToolState.activePresetKind : std::nullopt;
        mToolState.EnterEraser(presetToRestore);
        mStageAddActorPanel.CancelPlacement();
        mToolState.statusMessage = "消したいものをクリックしてください";
        return;
    }

    const std::optional<UGCPresetKind> presetToRestore = mToolState.presetBeforeEraser;
    const bool didRestorePreviousPreset = presetToRestore && mStageAddActorPanel.ActivateUGCPreset(*presetToRestore);
    mToolState.LeaveEraser(didRestorePreviousPreset);
    if (didRestorePreviousPreset) {
        mToolState.statusMessage = "置く状態に戻りました";
    } else {
        mToolState.statusMessage = "選択モードに戻りました";
    }
}

void UGCEditorInteractionController::ActivateSelectionMode()
{
    mToolState.ActivateSelection();
    mStageAddActorPanel.CancelPlacement();
    mToolState.statusMessage = "選びたいものをクリックしてください";
}

void UGCEditorInteractionController::ChangeLayer(int layerDelta)
{
    const bool isMovingSelection = mDragState.isDragging && mSelectionController.GetSelectedActorCount() > 0;
    const int previousLayer = mToolState.editLayer;
    ChangeEditLayer(layerDelta);
    if (mToolState.editLayer != previousLayer) {
        mEditorTutorial.RecordLayerChange(layerDelta, isMovingSelection);
    }
}

void UGCEditorInteractionController::MoveSelectionOnGrid(int gridX, int gridZ)
{
    if (mToolState.isEraserMode || mStageAddActorPanel.IsPlacementActive() ||
        mSelectionController.GetSelectedActorCount() == 0) {
        return;
    }
    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 movement(static_cast<float>(gridX) * gridSize, 0.0f, static_cast<float>(gridZ) * gridSize);
    std::vector<StageActorRef> selectedRefs;
    for (const StageActorInstance& selected : mSelectionController.CollectSelectedActorInstances()) {
        selectedRefs.push_back(selected.ref);
    }
    if (mSelectionController.IsMovingPlatformDestinationSelected()) {
        const bool destinationMoved = mEditCommandController.ExecuteEdit(
            [&]() { return mStageAddActorPanel.TryTranslateUGCMovingPlatformDestinations(selectedRefs, movement); });
        if (destinationMoved) {
            mSelectionController.Clear();
            mToolState.statusMessage = "移動先を1マス動かしました";
        } else {
            mToolState.statusMessage = "移動先を動かせませんでした";
        }
        return;
    }

    mSelectionController.MoveSelectedActorsByDelta(movement);
    mToolState.statusMessage = SaveSelectionMove(selectedRefs, movement, StageEditGrouping::NewOperation)
                                   ? "選んだものを1マス動かしました"
                                   : "移動を保存できませんでした";
}

bool UGCEditorInteractionController::SaveSelectionMove(const std::vector<StageActorRef>& actorRefs,
                                                       const glm::vec3& delta, StageEditGrouping grouping)
{
    UGCCellEditResult cellResult = UGCCellEditResult::Unchanged;
    const bool saved = mEditCommandController.ExecuteEdit(
        [&]() {
            YAML::Node config;
            if (!StageYamlRepository::LoadCurrentStage(mContext, config) ||
                !mStageActorYamlWriter.WriteEditorAuthoredTransforms(config) ||
                !StagePlanetYamlWriter(mContext).WriteEditorAuthoredTransforms(config)) {
                return false;
            }
            cellResult = mStageAddActorPanel.WriteTranslatedUGCCells(config, actorRefs, delta);
            UGCWorkMetadata::InvalidateClearVerification(config);
            return cellResult != UGCCellEditResult::Failed && StageYamlRepository::SaveCurrentStage(mContext, config) &&
                   mContext.game->ReloadCurrentStage();
        },
        grouping, StageEditFailureRecovery::RestoreRuntime);
    if (saved && cellResult == UGCCellEditResult::Unchanged) {
        std::unordered_set<std::string> keys;
        for (const auto& actorRef : actorRefs) {
            keys.insert(StageActorQuery::MakeKey(actorRef));
        }
        mSelectionController.SetSelectedKeys(keys);
    }
    return saved;
}

bool UGCEditorInteractionController::TryIntersectEditLayerPlane(const glm::vec3& rayFrom, const glm::vec3& rayTo,
                                                                float planeHeight, glm::vec3& outIntersection) const
{
    const glm::vec3 rayDirection = rayTo - rayFrom;
    if (std::abs(rayDirection.y) <= 0.000001f) {
        return false;
    }

    const float rayParameter = (planeHeight - rayFrom.y) / rayDirection.y;
    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        return false;
    }
    outIntersection = rayFrom + rayDirection * rayParameter;
    return true;
}

void UGCEditorInteractionController::UpdateSelectionDrag()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!mDragState.isDragging) {
        BeginSelectionDrag();
        return;
    }

    if (!isMouseDown) {
        FinishSelectionDrag();
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    glm::vec3 intersection;
    const float gridSize = mContext.game->GetUGCGridSize();
    const float planeHeight = static_cast<float>(mToolState.editLayer) * gridSize;
    if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo) ||
        !TryIntersectEditLayerPlane(rayFrom, rayTo, planeHeight, intersection)) {
        return;
    }

    const glm::ivec3 currentCell = UGCPlatformGrid::CalculateGridPosition(intersection, gridSize);
    glm::ivec3 horizontalCellDelta = currentCell - mDragState.dragStartCell;
    horizontalCellDelta.y = 0;
    const glm::ivec3 unappliedCellDelta = horizontalCellDelta - mDragState.appliedHorizontalCellDelta;
    if (unappliedCellDelta.x == 0 && unappliedCellDelta.z == 0) {
        return;
    }
    const glm::vec3 movementDelta(static_cast<float>(unappliedCellDelta.x) * gridSize, 0.0f,
                                  static_cast<float>(unappliedCellDelta.z) * gridSize);

    // クリックだけではUndoを増やさず、ドラッグ全体を1回の操作として記録する。
    if (!mDragState.hasMoved) {
        mDragState.hasMoved = true;
    }
    if (mDragState.isMovingPlatformDestination) {
        mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(movementDelta);
    } else {
        mSelectionController.MoveSelectedActorsByDelta(movementDelta);
    }
    mDragState.appliedHorizontalCellDelta = horizontalCellDelta;
    mDragState.appliedDelta += movementDelta;
}

void UGCEditorInteractionController::ChangeEditLayer(int layerDelta)
{
    constexpr int minimumLayer = 0;
    constexpr int maximumLayer = 20;
    const int nextLayer = std::clamp(mToolState.editLayer + layerDelta, minimumLayer, maximumLayer);
    if (nextLayer == mToolState.editLayer) {
        mToolState.statusMessage = layerDelta < 0 ? "ここがいちばん下です" : "これ以上高くできません";
        return;
    }

    const bool isMovingSelectedActors = mDragState.isDragging && mSelectionController.GetSelectedActorCount() > 0;
    const bool isMovingPlatformDestination = isMovingSelectedActors && mDragState.isMovingPlatformDestination;
    if (isMovingSelectedActors) {
        if (!mDragState.hasMoved) {
            mDragState.hasMoved = true;
        }
        const glm::vec3 layerMovement(
            0.0f, static_cast<float>(nextLayer - mToolState.editLayer) * mContext.game->GetUGCGridSize(), 0.0f);
        if (isMovingPlatformDestination) {
            const bool wasSaved = mEditCommandController.ExecuteEdit(
                [&]() {
                    return mStageAddActorPanel.TrySaveUGCMovingPlatformDestinationTranslation(mDragState.actorRefs,
                                                                                              layerMovement);
                },
                mDragState.hasSavedEdit ? StageEditGrouping::ContinueOperation : StageEditGrouping::NewOperation);
            if (!wasSaved) {
                mToolState.statusMessage = "移動先の高さを保存できませんでした";
                return;
            }
            mDragState.hasSavedEdit = true;
            mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(layerMovement);
            mDragState.savedDelta += layerMovement;
        } else {
            mSelectionController.MoveSelectedActorsByDelta(layerMovement);
            mDragState.appliedDelta += layerMovement;
        }
    }

    mToolState.editLayer = nextLayer;
    mStageAddActorPanel.SetUGCEditLayer(mToolState.editLayer);
    mSelectionController.SetUGCEditLayer(mToolState.editLayer);
    mContext.game->SetUGCPreviewEditLayer(mToolState.editLayer);
    if (!isMovingSelectedActors) {
        mSelectionController.Clear();
    }
    if (isMovingPlatformDestination) {
        mToolState.statusMessage = "移動先を" + std::to_string(mToolState.editLayer + 1) + "だん目へ動かしました";
    } else if (isMovingSelectedActors) {
        mToolState.statusMessage = "選んだものも" + std::to_string(mToolState.editLayer + 1) + "だん目へ動かしました";
    } else {
        mToolState.statusMessage = std::to_string(mToolState.editLayer + 1) + "だん目を作っています";
    }
}

void UGCEditorInteractionController::SyncEditLayerToPickedActor()
{
    Actor* pickedActor = mSelectionController.GetPickedActor();
    const std::optional<StageActorRef>& pickedRef = mSelectionController.GetPickedActorRef();
    if (!pickedActor || !pickedRef || !mContext.game) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const bool isMovingPlatformDestination = mSelectionController.IsMovingPlatformDestinationSelected();
    const glm::vec3 pickedPosition =
        isMovingPlatformDestination ? mSelectionController.CalculateSelectedActorsCenter() : pickedActor->GetPos();
    int pickedLayer = static_cast<int>(std::round(pickedPosition.y / gridSize));
    YAML::Node stageYaml;
    if (!isMovingPlatformDestination && StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        const YAML::Node sequence = stageYaml[pickedRef->sequenceName];
        if (sequence && sequence.IsSequence() && pickedRef->yamlIndex >= 0 &&
            pickedRef->yamlIndex < static_cast<int>(sequence.size())) {
            const YAML::Node actorNode = sequence[pickedRef->yamlIndex];
            if (actorNode["ugcGeneratedPlatform"] && actorNode["ugcGeneratedPlatform"].as<bool>(false)) {
                pickedLayer = actorNode["ugcGridLayer"].as<int>(pickedLayer);
            }
        }
    }

    constexpr int minimumLayer = 0;
    constexpr int maximumLayer = 20;
    pickedLayer = std::clamp(pickedLayer, minimumLayer, maximumLayer);
    if (pickedLayer == mToolState.editLayer) {
        return;
    }

    mToolState.editLayer = pickedLayer;
    mStageAddActorPanel.SetUGCEditLayer(mToolState.editLayer);
    mSelectionController.SetUGCEditLayer(mToolState.editLayer);
    mContext.game->SetUGCPreviewEditLayer(mToolState.editLayer);
    mToolState.statusMessage = std::to_string(mToolState.editLayer + 1) + "だん目のものを選びました";
}

void UGCEditorInteractionController::UpdateSceneInteraction()
{
    if (!mToolState.isEraserMode || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        mStageAddActorPanel.EndUGCEraseGesture();
    }
    if (mStageAddActorPanel.IsPlacementActive()) {
        mStageAddActorPanel.UpdatePlacement();
    } else {
        const bool isChoosingSwitchTarget = mConnectionState.HasPendingConnection();
        const bool allowsSelectionInteraction = !mToolState.isEraserMode && !isChoosingSwitchTarget;
        mSelectionController.SetBoxSelectionEnabled(allowsSelectionInteraction);
        mSelectionController.Update();
        if (!mToolState.isEraserMode && !isChoosingSwitchTarget && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::GetIO().KeyShift) {
            SyncEditLayerToPickedActor();
        }
        if (allowsSelectionInteraction && !mSelectionController.IsBoxSelectionGestureActive()) {
            UpdateSelectionDrag();
        }
        if (mToolState.isEraserMode && !isChoosingSwitchTarget && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            HandleEraserInteraction();
        }
    }
    mSelectionController.ApplyEditorSelectionFlags();
}

void UGCEditorInteractionController::AdjustZoom(float distanceMultiplier)
{
    mViewController.AdjustZoom(distanceMultiplier);
}

void UGCEditorInteractionController::ToggleVerticalView()
{
    mViewController.ToggleVerticalView();
}

void UGCEditorInteractionController::SetFixedView(const glm::vec3& viewDirection)
{
    mViewController.SetFixedView(viewDirection);
}

const glm::vec3& UGCEditorInteractionController::GetViewDirection() const
{
    return mViewController.GetViewDirection();
}

void UGCEditorInteractionController::HandleEraserInteraction()
{
    const auto tryDeletePickedPlanetOnly = [&]() {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            return false;
        }

        Actor* pickedActor = mSelectionController.GetPickedActor();
        const std::optional<StageActorRef>& pickedRef = mSelectionController.GetPickedActorRef();
        if (!pickedActor || !pickedRef || dynamic_cast<Planet*>(pickedActor) == nullptr ||
            pickedRef->type != StageActorType::Planet) {
            return false;
        }

        return mEditCommandController.DeletePlanetOnly(pickedRef->yamlIndex);
    };
    const auto tryDeletePickedActorOnCurrentLayer = [&]() {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || !mContext.game) {
            return false;
        }

        Actor* pickedActor = mSelectionController.GetPickedActor();
        const std::optional<StageActorRef>& pickedRef = mSelectionController.GetPickedActorRef();
        if (!pickedActor || !pickedRef || dynamic_cast<Planet*>(pickedActor) != nullptr) {
            return false;
        }

        Platform* pickedPlatform = dynamic_cast<Platform*>(pickedActor);
        if (pickedPlatform && pickedPlatform->GetIsUGCGenerated()) {
            return false;
        }
        const float gridSize = mContext.game->GetUGCGridSize();
        const int actorLayer = static_cast<int>(std::round(pickedActor->GetPos().y / gridSize));
        if (actorLayer != mToolState.editLayer) {
            return false;
        }

        return mEditCommandController.DeleteSelectedKeys({StageActorQuery::MakeKey(*pickedRef)});
    };

    const auto tryDeletePickedActorAsHighestFallback = [&]() {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            return false;
        }
        Actor* pickedActor = mSelectionController.GetPickedActor();
        const std::optional<StageActorRef>& pickedRef = mSelectionController.GetPickedActorRef();
        Platform* pickedPlatform = dynamic_cast<Platform*>(pickedActor);
        if (!pickedActor || !pickedRef || dynamic_cast<Planet*>(pickedActor) != nullptr ||
            (pickedPlatform && pickedPlatform->GetIsUGCGenerated())) {
            return false;
        }
        return mEditCommandController.DeleteSelectedKeys({StageActorQuery::MakeKey(*pickedRef)});
    };

    if (tryDeletePickedPlanetOnly()) {
        mToolState.statusMessage = "惑星だけを消しました";
        mEditorTutorial.RecordErase(true);
    } else if (tryDeletePickedActorOnCurrentLayer()) {
        mToolState.statusMessage = "今のだんのものを消しました";
        mEditorTutorial.RecordErase(true);
    } else if (mStageAddActorPanel.TryEraseUGCPlatformCell()) {
        mToolState.statusMessage = "足場を1マス消しました";
        mEditorTutorial.RecordErase(true);
    } else if (tryDeletePickedActorAsHighestFallback()) {
        mToolState.statusMessage = "いちばん上のものを消しました";
        mEditorTutorial.RecordErase(true);
    } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mSelectionController.GetSelectedKeys().empty()) {
        const std::optional<StageActorRef>& selectedRef = mSelectionController.GetPickedActorRef();
        if (!selectedRef || selectedRef->type != StageActorType::Planet) {
            const std::unordered_set<std::string> selectedKeys = mSelectionController.GetSelectedKeys();
            if (mEditCommandController.DeleteSelectedKeys(selectedKeys)) {
                mToolState.statusMessage = "選んだものを消しました";
                mEditorTutorial.RecordErase(true);
            }
        }
    }
}

void UGCEditorInteractionController::BeginSelectionDrag()
{
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::GetIO().WantCaptureMouse ||
        mSelectionController.GetSelectedActorCount() == 0) {
        return;
    }

    const std::vector<StageActorInstance> selectedInstances = mSelectionController.CollectSelectedActorInstances();
    if (selectedInstances.empty()) {
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const float planeHeight = static_cast<float>(mToolState.editLayer) * gridSize;
    const glm::vec3 selectionCenter = mSelectionController.CalculateSelectedActorsCenter();

    glm::vec3 intersection;
    if (!TryIntersectEditLayerPlane(rayFrom, rayTo, planeHeight, intersection)) {
        return;
    }

    mDragState.isDragging = true;
    mDragState.isMovingPlatformDestination = mSelectionController.IsMovingPlatformDestinationSelected();
    mDragState.hasMoved = false;
    mDragState.hasSavedEdit = false;
    mDragState.dragStartCell = UGCPlatformGrid::CalculateGridPosition(intersection, gridSize);
    mDragState.appliedHorizontalCellDelta = glm::ivec3(0);
    mDragState.initialCenter = selectionCenter;
    mDragState.appliedDelta = glm::vec3(0.0f);
    mDragState.savedDelta = glm::vec3(0.0f);
    mDragState.actorRefs.clear();
    mDragState.actorRefs.reserve(selectedInstances.size());
    for (const StageActorInstance& selectedInstance : selectedInstances) {
        mDragState.actorRefs.emplace_back(selectedInstance.ref);
    }
}

void UGCEditorInteractionController::FinishSelectionDrag()
{
    if (mDragState.hasMoved) {
        glm::vec3 totalDelta = mDragState.appliedDelta;
        if (mDragState.isMovingPlatformDestination) {
            const glm::vec3 finalDestinationCenter =
                mSelectionController.CalculateSelectedMovingPlatformDestinationsCenter();

            // レイヤー変更時に保存済みの差分を除き、終了時の二重適用を防ぐ。
            totalDelta = finalDestinationCenter - mDragState.initialCenter - mDragState.savedDelta;
        }
        const StageEditGrouping grouping =
            mDragState.hasSavedEdit ? StageEditGrouping::ContinueOperation : StageEditGrouping::NewOperation;
        bool saved = false;
        if (mDragState.isMovingPlatformDestination) {
            saved = glm::length(totalDelta) < 0.0001f ||
                    mEditCommandController.ExecuteEdit(
                        [&]() {
                            return mStageAddActorPanel.TryTranslateUGCMovingPlatformDestinations(mDragState.actorRefs,
                                                                                                 totalDelta);
                        },
                        grouping, StageEditFailureRecovery::RestoreRuntime);
            if (saved) {
                mSelectionController.Clear();
            }
        } else {
            saved = SaveSelectionMove(mDragState.actorRefs, totalDelta, grouping);
        }
        mToolState.statusMessage = saved ? "移動しました" : "移動を保存できませんでした";
        if (saved) {
            mEditorTutorial.RecordSelectionMove();
        }
    }
    mDragState.Reset();
}
