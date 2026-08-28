#include "gfx/debug/ugc/UGCEditorInteractionController.h"
#include "gfx/debug/ugc/UGCEditCommandController.h"
#include "gfx/debug/ugc/UGCEditLayerController.h"
#include "gfx/debug/ugc/UGCEditorViewController.h"
#include "gfx/debug/ugc/UGCSceneInteractionController.h"
#include "gfx/debug/ugc/UGCSelectionDragController.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCEditorTutorial.h"
#include "gfx/debug/ugc/UGCSelectionDragState.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"
#include "system/CameraSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

UGCEditorInteractionController::UGCEditorInteractionController(
    UGCEditCommandController& editCommandController,
    UGCEditLayerController& editLayerController,
    UGCEditorViewController& viewController,
    UGCSceneInteractionController& sceneInteractionController)
    : mEditCommandController(editCommandController),
      mEditLayerController(editLayerController),
      mViewController(viewController),
      mSceneInteractionController(sceneInteractionController)
{
}

UGCEditCommandController::UGCEditCommandController(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController,
    UGCEditorTutorial& editorTutorial,
    UGCEditorToolState& toolState,
    UGCSelectionDragState& dragState)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mEditorTutorial(editorTutorial),
      mToolState(toolState),
      mDragState(dragState)
{
}

void UGCEditCommandController::HandleUndo()
{
    const bool wasRestored = mEditCommandController.RestoreUndo();
    mEditorTutorial.RecordUndo(wasRestored);
    mToolState.statusMessage = wasRestored
        ? "1つ前の状態に戻しました"
        : "戻せる操作がありません";
}

void UGCEditCommandController::HandleRedo()
{
    mToolState.statusMessage = mEditCommandController.RestoreRedo()
        ? "戻した操作をやり直しました"
        : "やり直せる操作がありません";
}

void UGCEditCommandController::ToggleEraser()
{
    if (!mToolState.isEraserMode) {
        const std::optional<UGCPresetKind> presetToRestore =
            mStageAddActorPanel.IsPlacementActive()
            ? mToolState.activePresetKind
            : std::nullopt;
        mToolState.EnterEraser(presetToRestore);
        mStageAddActorPanel.CancelPlacement();
        mToolState.statusMessage = "消したいものをクリックしてください";
        return;
    }

    const std::optional<UGCPresetKind> presetToRestore =
        mToolState.presetBeforeEraser;
    const bool didRestorePreviousPreset =
        presetToRestore &&
        mStageAddActorPanel.ActivateUGCPreset(*presetToRestore);
    mToolState.LeaveEraser(didRestorePreviousPreset);
    if (didRestorePreviousPreset) {
        mToolState.statusMessage = "置く状態に戻りました";
    } else {
        mToolState.statusMessage = "選択モードに戻りました";
    }
}

void UGCEditCommandController::ActivateSelectionMode()
{
    mToolState.ActivateSelection();
    mStageAddActorPanel.CancelPlacement();
    mToolState.statusMessage = "選びたいものをクリックしてください";
}

UGCEditorViewController::UGCEditorViewController(
    DebugEditorContext& context,
    UGCEditorTutorial& editorTutorial)
    : mContext(context),
      mEditorTutorial(editorTutorial)
{
}

void UGCEditorViewController::AdjustZoom(float distanceMultiplier)
{
    AdjustViewDistance(distanceMultiplier);
    mEditorTutorial.RecordViewAdjustment();
}

UGCEditLayerController::UGCEditLayerController(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController,
    UGCEditorTutorial& editorTutorial,
    UGCEditorToolState& toolState,
    UGCSelectionDragState& dragState)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mEditorTutorial(editorTutorial),
      mToolState(toolState),
      mDragState(dragState)
{
}

void UGCEditLayerController::ChangeLayer(int layerDelta)
{
    const bool isMovingSelection =
        mDragState.isDragging &&
        mSelectionController.GetSelectedActorCount() > 0;
    const int previousLayer = mToolState.editLayer;
    ChangeEditLayer(layerDelta);
    if (mToolState.editLayer != previousLayer) {
        mEditorTutorial.RecordLayerChange(
            layerDelta,
            isMovingSelection);
    }
}

void UGCEditCommandController::MoveSelectionOnGrid(int gridX, int gridZ)
{
    if (mToolState.isEraserMode || mStageAddActorPanel.IsPlacementActive() ||
        mSelectionController.GetSelectedActorCount() == 0) {
        return;
    }
    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 movement(
        static_cast<float>(gridX) * gridSize, 0.0f,
        static_cast<float>(gridZ) * gridSize);
    mEditCommandController.PushUndo();
    std::vector<StageActorRef> selectedRefs;
    for (const StageActorInstance& selected :
         mSelectionController.CollectSelectedActorInstances()) {
        selectedRefs.push_back(selected.ref);
    }
    if (mSelectionController.IsMovingPlatformDestinationSelected()) {
        const bool destinationMoved =
            mStageAddActorPanel.TryTranslateUGCMovingPlatformDestinations(
                selectedRefs, movement);
        if (destinationMoved) {
            mSelectionController.Clear();
            mToolState.statusMessage = "移動先を1マス動かしました";
        } else {
            mToolState.statusMessage = "移動先を動かせませんでした";
        }
        return;
    }

    mSelectionController.MoveSelectedActorsByDelta(movement);
    const bool updatedUGCPlatform =
        mStageAddActorPanel.TryTranslateUGCPlatformCells(
            selectedRefs, movement);
    if (updatedUGCPlatform) {
        mSelectionController.Clear();
    }
    mToolState.statusMessage = "選んだものを1マス動かしました";
}

UGCSelectionDragController::UGCSelectionDragController(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageActorYamlWriter& stageActorYamlWriter,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController,
    UGCEditorTutorial& editorTutorial,
    UGCEditorToolState& toolState,
    UGCSelectionDragState& dragState)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mStageActorYamlWriter(stageActorYamlWriter),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mEditorTutorial(editorTutorial),
      mToolState(toolState),
      mDragState(dragState)
{
}

bool UGCSelectionDragController::TryIntersectDragPlane(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo,
    glm::vec3& outIntersection) const
{
    const glm::vec3 rayDirection = rayTo - rayFrom;
    const float denominator = glm::dot(
        rayDirection, mDragState.planeNormal);
    if (std::abs(denominator) <= 0.000001f) {
        return false;
    }

    const float rayParameter = glm::dot(
        mDragState.planePoint - rayFrom,
        mDragState.planeNormal) / denominator;
    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        return false;
    }
    outIntersection = rayFrom + rayDirection * rayParameter;
    return true;
}

void UGCSelectionDragController::Update()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    const bool isMouseDown =
        ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!mDragState.isDragging) {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::GetIO().WantCaptureMouse ||
            mSelectionController.GetSelectedActorCount() == 0) {
            return;
        }

        const std::vector<StageActorInstance> selectedInstances =
            mSelectionController.CollectSelectedActorInstances();
        if (selectedInstances.empty()) {
            return;
        }

        glm::vec3 rayFrom;
        glm::vec3 rayTo;
        if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo)) {
            return;
        }

        const CameraPose cameraPose =
            mContext.game->GetCameraSystem()->GetDebugCameraPose();
        glm::vec3 planeNormal = cameraPose.position - cameraPose.target;
        if (glm::length(planeNormal) <= 0.000001f) {
            return;
        }
        mDragState.planeNormal = glm::normalize(planeNormal);
        const glm::vec3 selectionCenter =
            mSelectionController.CalculateSelectedActorsCenter();
        mDragState.planePoint = selectionCenter;

        glm::vec3 intersection;
        if (!TryIntersectDragPlane(rayFrom, rayTo, intersection)) {
            return;
        }

        mDragState.isDragging = true;
        mDragState.isMovingPlatformDestination =
            mSelectionController.IsMovingPlatformDestinationSelected();
        mDragState.hasMoved = false;
        mDragState.offset =
            selectionCenter - intersection;
        mDragState.initialCenter = selectionCenter;
        mDragState.appliedDelta = glm::vec3(0.0f);
        mDragState.savedDelta = glm::vec3(0.0f);
        mDragState.actorRefs.clear();
        mDragState.actorRefs.reserve(selectedInstances.size());
        for (const StageActorInstance& selectedInstance : selectedInstances) {
            mDragState.actorRefs.emplace_back(selectedInstance.ref);
        }
        return;
    }

    if (!isMouseDown) {
        if (mDragState.hasMoved) {
            glm::vec3 totalDelta = mDragState.appliedDelta;
            if (mDragState.isMovingPlatformDestination) {
                const glm::vec3 finalDestinationCenter =
                    mSelectionController
                        .CalculateSelectedMovingPlatformDestinationsCenter();
                totalDelta = finalDestinationCenter -
                    mDragState.initialCenter -
                    mDragState.savedDelta;
            }
            bool updatedUGCPlatform = false;
            if (mDragState.isMovingPlatformDestination) {
                constexpr float minimumTranslationLength = 0.0001f;
                updatedUGCPlatform = glm::length(totalDelta) <
                    minimumTranslationLength;
                if (!updatedUGCPlatform) {
                    updatedUGCPlatform = mStageAddActorPanel
                        .TryTranslateUGCMovingPlatformDestinations(
                            mDragState.actorRefs, totalDelta);
                }
                if (!updatedUGCPlatform) {
                    mSelectionController
                        .MoveSelectedMovingPlatformDestinationsByDelta(
                            -totalDelta);
                }
            } else {
                mStageActorYamlWriter.SaveEditorAuthoredTransforms();
                updatedUGCPlatform =
                    mStageAddActorPanel.TryTranslateUGCPlatformCells(
                        mDragState.actorRefs, totalDelta);
            }
            if (updatedUGCPlatform) {
                mSelectionController.Clear();
            }
            mToolState.statusMessage = updatedUGCPlatform ||
                    !mDragState.isMovingPlatformDestination
                ? "移動しました"
                : "移動先を動かせませんでした";
            if (updatedUGCPlatform ||
                !mDragState.isMovingPlatformDestination) {
                mEditorTutorial.RecordSelectionMove();
            }
        }
        mDragState.isDragging = false;
        mDragState.isMovingPlatformDestination = false;
        mDragState.hasMoved = false;
        mDragState.appliedDelta = glm::vec3(0.0f);
        mDragState.savedDelta = glm::vec3(0.0f);
        mDragState.actorRefs.clear();
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    glm::vec3 intersection;
    if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo) ||
        !TryIntersectDragPlane(rayFrom, rayTo, intersection)) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 unsnappedTarget =
        intersection + mDragState.offset;
    const glm::vec3 unsnappedDelta =
        unsnappedTarget - mDragState.initialCenter;
    const glm::vec3 snappedTarget =
        mDragState.initialCenter + glm::vec3(
            std::round(unsnappedDelta.x / gridSize) * gridSize,
            std::round(unsnappedDelta.y / gridSize) * gridSize,
            std::round(unsnappedDelta.z / gridSize) * gridSize);
    const glm::vec3 currentCenter = mDragState.isMovingPlatformDestination
        ? mSelectionController
              .CalculateSelectedMovingPlatformDestinationsCenter()
        : mSelectionController.CalculateSelectedActorsCenter();
    const glm::vec3 movementDelta = snappedTarget - currentCenter;
    if (glm::length(movementDelta) <= 0.000001f) {
        return;
    }

    if (!mDragState.hasMoved) {
        mEditCommandController.PushUndo();
        mDragState.hasMoved = true;
    }
    if (mDragState.isMovingPlatformDestination) {
        mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(
            movementDelta);
    } else {
        mSelectionController.MoveSelectedActorsByDelta(movementDelta);
    }
    mDragState.appliedDelta += movementDelta;
}

void UGCEditLayerController::ChangeEditLayer(int layerDelta)
{
    constexpr int minimumLayer = 0;
    constexpr int maximumLayer = 20;
    const int nextLayer = std::clamp(
        mToolState.editLayer + layerDelta,
        minimumLayer,
        maximumLayer);
    if (nextLayer == mToolState.editLayer) {
        mToolState.statusMessage = layerDelta < 0
            ? "ここがいちばん下です"
            : "これ以上高くできません";
        return;
    }

    const bool isMovingSelectedActors =
        mDragState.isDragging &&
        mSelectionController.GetSelectedActorCount() > 0;
    const bool isMovingPlatformDestination =
        isMovingSelectedActors &&
        mDragState.isMovingPlatformDestination;
    if (isMovingSelectedActors) {
        if (!mDragState.hasMoved) {
            mEditCommandController.PushUndo();
            mDragState.hasMoved = true;
        }
        const glm::vec3 layerMovement(
            0.0f,
            static_cast<float>(nextLayer - mToolState.editLayer) *
                mContext.game->GetUGCGridSize(),
            0.0f);
        if (isMovingPlatformDestination) {
            const bool wasSaved = mStageAddActorPanel
                .TrySaveUGCMovingPlatformDestinationTranslation(
                    mDragState.actorRefs, layerMovement);
            if (!wasSaved) {
                mToolState.statusMessage = "移動先の高さを保存できませんでした";
                return;
            }
            mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(
                layerMovement);
            mDragState.savedDelta += layerMovement;
        } else {
            mSelectionController.MoveSelectedActorsByDelta(layerMovement);
            mDragState.appliedDelta += layerMovement;
        }

        mDragState.planePoint += layerMovement;
    }

    mToolState.editLayer = nextLayer;
    mStageAddActorPanel.SetUGCEditLayer(mToolState.editLayer);
    mSelectionController.SetUGCEditLayer(mToolState.editLayer);
    mContext.game->SetUGCPreviewEditLayer(mToolState.editLayer);
    if (!isMovingSelectedActors) {
        mSelectionController.Clear();
    }
    if (isMovingPlatformDestination) {
        mToolState.statusMessage = "移動先を" + std::to_string(mToolState.editLayer + 1) +
            "だん目へ動かしました";
    } else if (isMovingSelectedActors) {
        mToolState.statusMessage = "選んだものも" + std::to_string(mToolState.editLayer + 1) +
            "だん目へ動かしました";
    } else {
        mToolState.statusMessage = std::to_string(mToolState.editLayer + 1) +
            "だん目を作っています";
    }
}

void UGCEditLayerController::SyncEditLayerToPickedActor()
{
    Actor* pickedActor = mSelectionController.GetPickedActor();
    const std::optional<StageActorRef>& pickedRef =
        mSelectionController.GetPickedActorRef();
    if (!pickedActor || !pickedRef || !mContext.game) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const bool isMovingPlatformDestination =
        mSelectionController.IsMovingPlatformDestinationSelected();
    const glm::vec3 pickedPosition = isMovingPlatformDestination
        ? mSelectionController.CalculateSelectedActorsCenter()
        : pickedActor->GetPos();
    int pickedLayer = static_cast<int>(std::round(
        pickedPosition.y / gridSize));
    YAML::Node stageYaml;
    if (!isMovingPlatformDestination &&
        StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        const YAML::Node sequence = stageYaml[pickedRef->sequenceName];
        if (sequence && sequence.IsSequence() &&
            pickedRef->yamlIndex >= 0 &&
            pickedRef->yamlIndex < static_cast<int>(sequence.size())) {
            const YAML::Node actorNode = sequence[pickedRef->yamlIndex];
            if (actorNode["ugcGeneratedPlatform"] &&
                actorNode["ugcGeneratedPlatform"].as<bool>(false)) {
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
    mToolState.statusMessage = std::to_string(mToolState.editLayer + 1) +
        "だん目のものを選びました";
}

void UGCEditorViewController::ToggleVerticalView()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    mContext.game->ToggleUGCPreviewVerticalView();
    const bool isViewedFromBelow =
        mContext.game->GetIsUGCPreviewViewedFromBelow();
    const glm::vec3 viewDirection = isViewedFromBelow
        ? glm::vec3(0.0f, -1.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    CameraPose pose = cameraSystem->GetDebugCameraPose();
    const float currentViewDistance = glm::length(
        pose.position - pose.target);
    constexpr float fallbackViewDistance = 30.0f;
    const float viewDistance = currentViewDistance > 0.0001f
        ? currentViewDistance
        : fallbackViewDistance;

    pose.position = pose.target + viewDirection * viewDistance;
    pose.up = isViewedFromBelow
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 0.0f, -1.0f);
    cameraSystem->SetDebugCameraPose(pose);

    mViewDirection = viewDirection;
    mContext.game->SetIsUGCOrthographicView(true);
    mContext.game->SetFreeCameraMode(true);
}

void UGCEditorViewController::SetFixedView(const glm::vec3& viewDirection)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() ||
        !mContext.game->GetCurrentStage()) {
        return;
    }

    glm::vec3 stageCenter(0.0f);
    float viewDistance = 24.0f;
    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (!planets.empty()) {
        for (const Planet* planet : planets) {
            if (planet) stageCenter += planet->GetPos();
        }
        stageCenter /= static_cast<float>(planets.size());
        for (const Planet* planet : planets) {
            if (!planet) continue;
            viewDistance = std::max(
                viewDistance,
                glm::length(planet->GetPos() - stageCenter) +
                    std::max({
                        std::abs(planet->GetScale().x),
                        std::abs(planet->GetScale().y),
                        std::abs(planet->GetScale().z)}) * 3.0f);
        }
    }

    const glm::vec3 normalizedDirection = glm::normalize(viewDirection);
    const glm::vec3 absoluteDirection = glm::abs(normalizedDirection);
    const int activeAxisCount =
        (absoluteDirection.x > 0.001f ? 1 : 0) +
        (absoluteDirection.y > 0.001f ? 1 : 0) +
        (absoluteDirection.z > 0.001f ? 1 : 0);
    const bool isAxisAlignedView = activeAxisCount == 1;
    mViewDirection = normalizedDirection;
    CameraPose pose;
    pose.position = stageCenter + normalizedDirection * viewDistance;
    pose.target = stageCenter;
    if (normalizedDirection.y > 0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, -1.0f);
    } else if (normalizedDirection.y < -0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        pose.up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    pose.fieldOfViewDegrees = 55.0f;
    mContext.game->SetIsUGCOrthographicView(isAxisAlignedView);
    if (isAxisAlignedView) {
        const float matchingPerspectiveHalfHeight =
            viewDistance * std::tan(
                glm::radians(pose.fieldOfViewDegrees) * 0.5f);
        mContext.game->SetUGCOrthographicHalfHeight(
            matchingPerspectiveHalfHeight);
    }
    mContext.game->GetCameraSystem()->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
}

void UGCEditorViewController::AdjustViewDistance(float distanceMultiplier)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() ||
        !mContext.game->GetCurrentStage() ||
        distanceMultiplier <= 0.0f) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    if (mContext.game->GetIsUGCOrthographicView()) {
        constexpr float minimumHalfHeight = 1.0f;
        constexpr float maximumHalfHeight = 250.0f;
        const float nextHalfHeight = glm::clamp(
            mContext.game->GetUGCOrthographicHalfHeight() *
                distanceMultiplier,
            minimumHalfHeight,
            maximumHalfHeight);
        mContext.game->SetUGCOrthographicHalfHeight(nextHalfHeight);
        return;
    }

    CameraPose pose = cameraSystem->GetDebugCameraPose();

    const glm::vec3 targetToCamera = pose.position - pose.target;
    const float currentDistance = glm::length(targetToCamera);
    constexpr float minimumDistance = 3.0f;
    constexpr float maximumDistance = 250.0f;
    if (currentDistance <= 0.0001f) {
        return;
    }

    const float nextDistance = glm::clamp(
        currentDistance * distanceMultiplier,
        minimumDistance,
        maximumDistance);
    pose.position = pose.target +
        targetToCamera / currentDistance * nextDistance;
    cameraSystem->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
}

const glm::vec3& UGCEditorViewController::GetViewDirection() const
{
    return mViewDirection;
}


UGCSceneInteractionController::UGCSceneInteractionController(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController,
    UGCEditorTutorial& editorTutorial,
    UGCEditorToolState& toolState,
    UGCSwitchConnectionState& connectionState,
    UGCSelectionDragController& selectionDragController,
    UGCEditLayerController& editLayerController)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController),
      mEditorTutorial(editorTutorial),
      mToolState(toolState),
      mConnectionState(connectionState),
      mSelectionDragController(selectionDragController),
      mEditLayerController(editLayerController)
{
}

void UGCSceneInteractionController::Update()
{
    if (mStageAddActorPanel.IsPlacementActive()) {
        mStageAddActorPanel.UpdatePlacement();
    } else {
        const bool isChoosingSwitchTarget =
            mConnectionState.HasPendingConnection();
        const bool allowsSelectionInteraction =
            !mToolState.isEraserMode && !isChoosingSwitchTarget;
        mSelectionController.SetBoxSelectionEnabled(
            allowsSelectionInteraction);
        mSelectionController.Update();
        if (!mToolState.isEraserMode &&
            !isChoosingSwitchTarget &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::GetIO().KeyShift) {
            mEditLayerController.SyncEditLayerToPickedActor();
        }
        if (allowsSelectionInteraction &&
            !mSelectionController.IsBoxSelectionGestureActive()) {
            mSelectionDragController.Update();
        }
        if (mToolState.isEraserMode && !isChoosingSwitchTarget &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const auto tryDeletePickedPlanetOnly = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    return false;
                }

                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) == nullptr ||
                    pickedRef->type != StageActorType::Planet) {
                    return false;
                }

                return mEditCommandController.DeletePlanetOnly(
                    pickedRef->yamlIndex);
            };
            const auto tryDeletePickedActorOnCurrentLayer = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
                    !mContext.game) {
                    return false;
                }

                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) != nullptr) {
                    return false;
                }




                Platform* pickedPlatform = dynamic_cast<Platform*>(pickedActor);
                if (pickedPlatform && pickedPlatform->GetIsUGCGenerated()) {
                    return false;
                }
                const float gridSize = mContext.game->GetUGCGridSize();
                const int actorLayer = static_cast<int>(std::round(
                    pickedActor->GetPos().y / gridSize));
                if (actorLayer != mToolState.editLayer) {
                    return false;
                }

                return mEditCommandController.DeleteSelectedKeys({
                    StageActorQuery::MakeKey(*pickedRef)});
            };

            const auto tryDeletePickedActorAsHighestFallback = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    return false;
                }
                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                Platform* pickedPlatform =
                    dynamic_cast<Platform*>(pickedActor);
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) != nullptr ||
                    (pickedPlatform && pickedPlatform->GetIsUGCGenerated())) {
                    return false;
                }
                return mEditCommandController.DeleteSelectedKeys({
                    StageActorQuery::MakeKey(*pickedRef)});
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
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                       !mSelectionController.GetSelectedKeys().empty()) {
                const std::optional<StageActorRef>& selectedRef =
                    mSelectionController.GetPickedActorRef();
                if (!selectedRef ||
                    selectedRef->type != StageActorType::Planet) {
                    const std::unordered_set<std::string> selectedKeys =
                        mSelectionController.GetSelectedKeys();
                    if (mEditCommandController.DeleteSelectedKeys(selectedKeys)) {
                        mToolState.statusMessage = "選んだものを消しました";
                        mEditorTutorial.RecordErase(true);
                    }
                }
            }
        }
    }
    mSelectionController.ApplyEditorSelectionFlags();
}

void UGCEditorInteractionController::HandleUndo()
{
    mEditCommandController.HandleUndo();
}

void UGCEditorInteractionController::HandleRedo()
{
    mEditCommandController.HandleRedo();
}

void UGCEditorInteractionController::ToggleEraser()
{
    mEditCommandController.ToggleEraser();
}

void UGCEditorInteractionController::ActivateSelectionMode()
{
    mEditCommandController.ActivateSelectionMode();
}

void UGCEditorInteractionController::AdjustZoom(float distanceMultiplier)
{
    mViewController.AdjustZoom(distanceMultiplier);
}

void UGCEditorInteractionController::ChangeLayer(int layerDelta)
{
    mEditLayerController.ChangeLayer(layerDelta);
}

void UGCEditorInteractionController::MoveSelectionOnGrid(int gridX, int gridZ)
{
    mEditCommandController.MoveSelectionOnGrid(gridX, gridZ);
}

void UGCEditorInteractionController::UpdateSceneInteraction()
{
    mSceneInteractionController.Update();
}

void UGCEditorInteractionController::ChangeEditLayer(int layerDelta)
{
    mEditLayerController.ChangeEditLayer(layerDelta);
}

void UGCEditorInteractionController::ToggleVerticalView()
{
    mViewController.ToggleVerticalView();
}

void UGCEditorInteractionController::SetFixedView(
    const glm::vec3& viewDirection)
{
    mViewController.SetFixedView(viewDirection);
}

const glm::vec3& UGCEditorInteractionController::GetViewDirection() const
{
    return mViewController.GetViewDirection();
}
