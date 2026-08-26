#include "gfx/debug/stage/StageActorPlacementController.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementResolver.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/stage/UGCPlatformGrid.h"
#include "gfx/debug/stage/UGCPlatformCellService.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"

#include <utility>
#include <vector>

StageActorPlacementController::StageActorPlacementController(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    UGCPlatformCellService& platformCellService,
    StageActorPlacementResolver& placementResolver)
    : mContext(context),
      mCreateService(actorCreateService),
      mUGCPlatformCellService(platformCellService),
      mPlacementResolver(placementResolver)
{
}

void StageActorPlacementController::SetSelectionController(
    StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void StageActorPlacementController::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mPushUndoCallback = std::move(pushUndoCallback);
}

bool StageActorPlacementController::BeginDuplicatePlacement(
    const StageActorRef& sourceRef)
{
    if (!mContext.game ||
        !mContext.game->GetCurrentStage() ||
        !mSelectionController ||
        sourceRef.sequenceName.empty() ||
        sourceRef.yamlIndex < 0) {
        return false;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext, stageYaml)) {
        return false;
    }

    const YAML::Node sourceSequence =
        stageYaml[sourceRef.sequenceName];
    if (!sourceSequence ||
        !sourceSequence.IsSequence() ||
        sourceRef.yamlIndex >=
            static_cast<int>(sourceSequence.size())) {
        return false;
    }

    const YAML::Node sourceNode =
        sourceSequence[sourceRef.yamlIndex];
    if (!sourceNode || !sourceNode.IsMap()) {
        return false;
    }

    Actor* sourceActor = StageActorQuery::FindActorByRef(
        mContext.game->GetCurrentStage(), sourceRef);
    const int fallbackPlanetIndex =
        mPlacementResolver.ResolvePlanetIndex(sourceActor, 0);
    const YAML::Node sourceTemplate = YAML::Clone(sourceNode);
    const std::string displayName =
        StageActorQuery::GetTypeLabel(sourceRef) +
        "（選択中の設定）";

    BeginPlacement(
        displayName,
        fallbackPlanetIndex,
        [this, sourceRef, sourceTemplate](
            int planetIndex,
            const StageActorPlacement& placement) {
            if (mPushUndoCallback) {
                mPushUndoCallback();
            }

            return mCreateService.DuplicateActorAtPlacement(
                sourceRef,
                sourceTemplate,
                planetIndex,
                placement);
        });
    return true;
}

void StageActorPlacementController::BeginPlacement(
    const std::string& displayName,
    int fallbackPlanetIndex,
    std::function<bool(int, const StageActorPlacement&)> placementCreator,
    bool snapToGridIntersections,
    bool continuousPlacement,
    bool autoStackUGCPlatforms,
    bool showUGCPlatformPreview)
{
    mPlacementDisplayName = displayName;
    mPlacementFallbackPlanetIndex = fallbackPlanetIndex;
    mPlacementCreator = std::move(placementCreator);
    mSnapPlacementToGridIntersections = snapToGridIntersections;
    mIsContinuousPlacement = continuousPlacement;
    mAutoStackUGCPlatforms = autoStackUGCPlatforms;
    mShowUGCPlatformPreview = showUGCPlatformPreview ||
        (mContext.game && mContext.game->GetIsUGCMode());
    mUGCPlacementPreviewModelPath.clear();
    mUGCPlacementPreviewModelScale = glm::vec3(1.0f);
    mIsContinuousPlacementStrokeActive = false;
    mUGCContinuousPlacementLayer.reset();
    mLastPaintedUGCCell.reset();
    mPlacementPreviewPosition.reset();
    mPlacementStatus = "ゲーム画面をクリックして配置してください";
}

void StageActorPlacementController::CancelPlacement()
{
    mPlacementCreator = {};
    mPlacementDisplayName.clear();
    mPlacementFallbackPlanetIndex = -1;
    mSnapPlacementToGridIntersections = true;
    mIsContinuousPlacement = false;
    mAutoStackUGCPlatforms = false;
    mShowUGCPlatformPreview = false;
    mUGCPlacementPreviewModelPath.clear();
    mIsContinuousPlacementStrokeActive = false;
    mUGCContinuousPlacementLayer.reset();
    mLastPaintedUGCCell.reset();
    mPlacementPreviewPosition.reset();
    if (mContext.game) {
        mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
        mContext.game->SetUGCPlacementModelPreview(std::nullopt);
    }
    mPlacementStatus = "連続配置を終了しました";
}

void StageActorPlacementController::UpdatePlacement()
{
    if (!mPlacementCreator) {
        return;
    }



    // ポインターが有効な組立領域を外れた後もゴーストが残らないよう、毎フレーム先に消去する。
    mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
    mContext.game->SetUGCPlacementModelPreview(std::nullopt);

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        CancelPlacement();
        return;
    }

    const bool isMouseDown =
        ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!isMouseDown) {
        mIsContinuousPlacementStrokeActive = false;
        mUGCContinuousPlacementLayer.reset();
        mLastPaintedUGCCell.reset();
    }

    if (ImGui::GetIO().WantCaptureMouse) {
        mPlacementPreviewPosition.reset();
        return;
    }

    if (!mSelectionController || !mContext.game ||
        !mContext.game->GetPhysicsSystem()) {
        mPlacementStatus = "配置に必要なシステムを利用できません";
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    StageActorPlacement placement;
    int planetIndex = mPlacementFallbackPlanetIndex;
    if (mContext.game->GetIsUGCMode()) {
        planetIndex = 0;
        if (!mPlacementResolver.TryResolveUGCBuildPlanePlacement(
                rayFrom, rayTo, mUGCEditLayer, placement)) {
            mPlacementStatus = "クリック位置に配置点を作れませんでした";
            return;
        }
    } else {
        const std::optional<PhysicsSystem::RayHitActor> hit =
            mContext.game->GetPhysicsSystem()->RaycastStageSurface(
                rayFrom, rayTo);
        if (!hit) {
            mPlacementStatus =
                "配置できる惑星・足場・ステージモデルに当たりませんでした";
            return;
        }
        planetIndex = mPlacementResolver.ResolvePlanetIndex(
            hit->actor,
            mPlacementFallbackPlanetIndex);
        placement.worldPosition = hit->hitPos;
        placement.surfaceNormal = hit->hitNormal;
    }

    if (planetIndex < 0) {
        mPlacementStatus = "クリックした面の所属惑星を特定できませんでした";
        return;
    }

    if (mContext.game->GetIsUGCMode() &&
        mSnapPlacementToGridIntersections) {
        // 保存するセルはグリッド角だが、生成される足場はセル中心に置かれる。ゴーストも完成後の位置を表示する。
        const float gridSize = mContext.game->GetUGCGridSize();
        placement.worldPosition = UGCPlatformGrid::SnapToGridIntersections(
            placement.worldPosition, gridSize, mUGCEditLayer);
    }

    if (mContext.game->GetIsUGCMode() && mAutoStackUGCPlatforms) {
        const float gridSize = mContext.game->GetUGCGridSize();
        const int placementLayer = mUGCContinuousPlacementLayer
            ? *mUGCContinuousPlacementLayer
            : mUGCPlatformCellService.ResolvePlacementLayerAtGridPosition(
                planetIndex,
                placement.worldPosition,
                gridSize,
                mUGCEditLayer);
        if (isMouseDown && !mUGCContinuousPlacementLayer) {
            mUGCContinuousPlacementLayer = placementLayer;
        }
        placement.worldPosition.y =
            static_cast<float>(placementLayer) * gridSize;
    }

    glm::vec3 previewPosition = placement.worldPosition;
    if (mContext.game->GetIsUGCMode() && mShowUGCPlatformPreview) {



        const float gridSize = mContext.game->GetUGCGridSize();
        glm::vec3 previewModelScale = mUGCPlacementPreviewModelScale;
        const bool usesPlatformFootprint =
            mPlacementDisplayName == "通常足場" ||
            mPlacementDisplayName == "消える足場" ||
            mPlacementDisplayName == "くっつき足場" ||
            mPlacementDisplayName == "移動足場：出発点" ||
            mPlacementDisplayName == "移動足場：到着点";
        if (usesPlatformFootprint) {
            previewPosition =
                UGCPlatformGrid::CalculateFootprintPreviewPosition(
                    placement.worldPosition,
                    gridSize,
                    mUGCPlatformFootprintSideLength);
            previewModelScale =
                UGCPlatformGrid::CalculateFootprintPreviewScale(
                    gridSize, mUGCPlatformFootprintSideLength);
        } else {
            previewPosition = UGCPlatformGrid::CalculateCellPreviewPosition(
                placement.worldPosition, gridSize);
            placement.worldPosition = previewPosition;
        }
        mContext.game->SetUGCPlatformPlacementPreview(previewPosition);
        mContext.game->SetUGCPlacementModelPreview(
            previewPosition,
            mUGCPlacementPreviewModelPath,
            previewModelScale);
    }
    mPlacementPreviewPosition = previewPosition;
    const bool placementInputTriggered = mIsContinuousPlacement
        ? isMouseDown
        : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (!placementInputTriggered) {
        return;
    }

    if (mIsContinuousPlacement) {
        const float gridSize = mContext.game->GetUGCGridSize();
        const glm::ivec3 paintedCell =
            UGCPlatformGrid::CalculateGridPosition(
                placement.worldPosition, gridSize);
        if (mLastPaintedUGCCell &&
            *mLastPaintedUGCCell == paintedCell) {
            return;
        }
        if (!mIsContinuousPlacementStrokeActive) {
            if (mPushUndoCallback) {
                mPushUndoCallback();
            }
            mIsContinuousPlacementStrokeActive = true;
        }
        mLastPaintedUGCCell = paintedCell;
    }

    const bool created = mPlacementCreator(planetIndex, placement);
    mPlacementStatus = created
                           ? mPlacementDisplayName + "を配置しました。続けてクリックできます"
                           : mPlacementDisplayName + "の配置に失敗しました";
}
