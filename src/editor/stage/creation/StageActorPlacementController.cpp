#include "editor/stage/creation/StageActorPlacementController.h"

#include "actor/Actor.h"
#include "actor/Platform.h"
#include "app/Game.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/creation/StageActorCreateService.h"
#include "editor/stage/creation/StageActorPlacementResolver.h"
#include "editor/stage/persistence/StageYamlRepository.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "editor/stage/ugc/UGCPlatformCellService.h"
#include "editor/stage/ugc/UGCPlatformGrid.h"
#include "imgui.h"
#include "system/PhysicsSystem.h"
#include "world/Stage.h"

#include <algorithm>
#include <utility>
#include <vector>

StageActorPlacementController::StageActorPlacementController(DebugEditorContext& context,
                                                             StageActorCreateService& actorCreateService,
                                                             UGCPlatformCellService& platformCellService,
                                                             StageActorPlacementResolver& placementResolver)
    : mContext(context),
      mCreateService(actorCreateService),
      mUGCPlatformCellService(platformCellService),
      mPlacementResolver(placementResolver)
{
}

void StageActorPlacementController::SetSelectionController(StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void StageActorPlacementController::SetEditExecutor(StageEditExecutor executeEdit)
{
    mExecuteEdit = std::move(executeEdit);
}

bool StageActorPlacementController::BeginDuplicatePlacement(const StageActorRef& sourceRef)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mSelectionController ||
        sourceRef.sequenceName.empty() || sourceRef.yamlIndex < 0) {
        return false;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return false;
    }

    const YAML::Node sourceSequence = stageYaml[sourceRef.sequenceName];
    if (!sourceSequence || !sourceSequence.IsSequence() ||
        sourceRef.yamlIndex >= static_cast<int>(sourceSequence.size())) {
        return false;
    }

    const YAML::Node sourceNode = sourceSequence[sourceRef.yamlIndex];
    if (!sourceNode || !sourceNode.IsMap()) {
        return false;
    }

    Actor* sourceActor = StageActorQuery::FindActorByRef(mContext.game->GetCurrentStage(), sourceRef);
    const int fallbackPlanetIndex = mPlacementResolver.ResolvePlanetIndex(sourceActor, 0);
    const YAML::Node sourceTemplate = YAML::Clone(sourceNode);
    const std::string displayName = StageActorQuery::GetTypeLabel(sourceRef) + "（選択中の設定）";

    BeginPlacement(displayName, fallbackPlanetIndex,
                   [this, sourceRef, sourceTemplate](int planetIndex, const StageActorPlacement& placement) {
                       return mCreateService.DuplicateActorAtPlacement(sourceRef, sourceTemplate, planetIndex,
                                                                       placement);
                   });
    return true;
}

void StageActorPlacementController::BeginPlacement(
    const std::string& displayName, int fallbackPlanetIndex,
    std::function<bool(int, const StageActorPlacement&)> placementCreator, const StagePlacementOptions& options)
{
    ClearMovingPlatformPlacementWorkflow();
    mPlacementDisplayName = displayName;
    mPlacementFallbackPlanetIndex = fallbackPlanetIndex;
    mPlacementCreator = std::move(placementCreator);
    mTargetPlatformSelector = {};
    mPlacementWithoutTargetCreator = {};
    mContinuePlacementCallback = {};
    mPlacementOptions = options;
    mPlacementOptions.showUGCPlatformPreview |= mContext.game && mContext.game->GetIsUGCMode();
    mUGCPlacementPreviewModelPath.clear();
    mUGCPlacementPreviewModelScale = glm::vec3(1.0f);
    mIsContinuousPlacementStrokeActive = false;
    mHasSavedPlacementStroke = false;
    mUGCContinuousPlacementLayer.reset();
    mLastPlacementAttemptCell.reset();
    mPlacementPreviewPosition.reset();
    mMovingPlatformPathStartPosition.reset();
    mTargetSelectionSourcePreviewPosition.reset();
    mFixedPlacementPreviewPositions.clear();
    mPlacementStatus = "ゲーム画面をクリックして配置してください";
}

void StageActorPlacementController::BeginTargetPlatformSelection(const std::string& displayName,
                                                                 const glm::vec3& sourcePreviewPosition,
                                                                 std::function<bool(Platform*)> targetSelector,
                                                                 std::function<bool()> placementWithoutTargetCreator,
                                                                 std::function<void()> continuePlacementCallback)
{
    ClearMovingPlatformPlacementWorkflow();
    mPlacementCreator = {};
    mTargetPlatformSelector = std::move(targetSelector);
    mPlacementWithoutTargetCreator = std::move(placementWithoutTargetCreator);
    mContinuePlacementCallback = std::move(continuePlacementCallback);
    mPlacementDisplayName = displayName;
    mTargetSelectionSourcePreviewPosition = sourcePreviewPosition;
    mPlacementPreviewPosition.reset();
    mPlacementStatus = "対応する足場を選択してください（足場以外で接続せず配置）";
}

void StageActorPlacementController::BeginMovingPlatformStrokePlacement(
    std::function<bool(int, const std::vector<glm::ivec3>&, const glm::ivec3&)> movingPlatformCreator)
{
    mPlacementCreator = {};
    mTargetPlatformSelector = {};
    mPlacementWithoutTargetCreator = {};
    mContinuePlacementCallback = {};
    mMovingPlatformCreator = std::move(movingPlatformCreator);
    mMovingPlatformStartCells.clear();
    mWasMovingPlatformStrokeActive = false;
    mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::SourceStroke;
    mPlacementPreviewPosition.reset();
    mMovingPlatformPathStartPosition.reset();
    mTargetSelectionSourcePreviewPosition.reset();
    mFixedPlacementPreviewPositions.clear();
    mPlacementDisplayName = "移動足場：出発地点を長押しで配置";
    mPlacementStatus = "長押しして出発側の足場を配置してください";
}

void StageActorPlacementController::CancelPlacement()
{
    mPlacementCreator = {};
    mTargetPlatformSelector = {};
    mPlacementWithoutTargetCreator = {};
    mContinuePlacementCallback = {};
    ClearMovingPlatformPlacementWorkflow();
    mPlacementDisplayName.clear();
    mPlacementFallbackPlanetIndex = -1;
    mPlacementOptions.snapToGridIntersections = true;
    mPlacementOptions.continuousPlacement = false;
    mPlacementOptions.autoStackUGCPlatforms = false;
    mPlacementOptions.showUGCPlatformPreview = false;
    mUGCPlacementPreviewModelPath.clear();
    mIsContinuousPlacementStrokeActive = false;
    mHasSavedPlacementStroke = false;
    mUGCContinuousPlacementLayer.reset();
    mLastPlacementAttemptCell.reset();
    mPlacementPreviewPosition.reset();
    mMovingPlatformPathStartPosition.reset();
    mTargetSelectionSourcePreviewPosition.reset();
    mFixedPlacementPreviewPositions.clear();
    if (mContext.game) {
        mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
        mContext.game->SetUGCPlacementModelPreview(std::nullopt);
        mContext.game->SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);
    }
    mPlacementStatus = "連続配置を終了しました";
}

void StageActorPlacementController::ClearMovingPlatformPlacementWorkflow()
{
    mMovingPlatformCreator = {};
    mMovingPlatformStartCells.clear();
    mWasMovingPlatformStrokeActive = false;
    mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::SourceStroke;
}

void StageActorPlacementController::UpdatePlacement()
{
    if (!mContext.game) {
        mPlacementStatus = "配置に必要なシステムを利用できません";
        return;
    }
    if (mMovingPlatformCreator) {
        UpdateMovingPlatformStrokePlacement();
        return;
    }
    if (mTargetPlatformSelector) {
        UpdateTargetPlatformSelection();
        return;
    }

    if (!mPlacementCreator) {
        return;
    }

    // ポインターが有効な組立領域を外れた後もゴーストが残らないよう、毎フレーム先に消去する。
    mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
    mContext.game->SetUGCPlacementModelPreview(std::nullopt);
    mContext.game->SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        CancelPlacement();
        return;
    }

    const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!isMouseDown) {
        mIsContinuousPlacementStrokeActive = false;
        mHasSavedPlacementStroke = false;
        mUGCContinuousPlacementLayer.reset();
        mLastPlacementAttemptCell.reset();
    }

    if (ImGui::GetIO().WantCaptureMouse) {
        mPlacementPreviewPosition.reset();
        return;
    }

    if (!mSelectionController || !mContext.game || !mContext.game->GetPhysicsSystem()) {
        mPlacementStatus = "配置に必要なシステムを利用できません";
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    StageActorPlacement placement;
    int planetIndex = 0;
    if (mContext.game->GetIsUGCMode()) {
        if (!mPlacementResolver.TryResolveUGCBuildPlanePlacement(rayFrom, rayTo, mUGCEditLayer, placement)) {
            mPlacementStatus = "クリック位置に配置点を作れませんでした";
            return;
        }
    } else {
        const std::optional<PhysicsSystem::RayHitActor> hit =
            mContext.game->GetPhysicsSystem()->RaycastStageSurface(rayFrom, rayTo);
        if (!hit) {
            mPlacementStatus = "配置できる惑星・足場・ステージモデルに当たりませんでした";
            return;
        }
        planetIndex = mPlacementResolver.ResolvePlanetIndex(hit->actor, mPlacementFallbackPlanetIndex);
        placement.worldPosition = hit->hitPos;
        placement.surfaceNormal = hit->hitNormal;
    }

    if (planetIndex < 0) {
        mPlacementStatus = "クリックした面の所属惑星を特定できませんでした";
        return;
    }

    const bool usesPlatformFootprint = mPlacementOptions.footprint == StagePlacementFootprint::Platform;
    if (mContext.game->GetIsUGCMode() && mPlacementOptions.snapToGridIntersections && !usesPlatformFootprint) {
        const float gridSize = mContext.game->GetUGCGridSize();
        placement.worldPosition =
            UGCPlatformGrid::SnapToGridIntersections(placement.worldPosition, gridSize, mUGCEditLayer);
    }

    if (mContext.game->GetIsUGCMode() && mPlacementOptions.autoStackUGCPlatforms) {
        const float gridSize = mContext.game->GetUGCGridSize();
        const int placementLayer = mUGCContinuousPlacementLayer
                                       ? *mUGCContinuousPlacementLayer
                                       : mUGCPlatformCellService.ResolvePlacementLayerAtGridPosition(
                                             planetIndex, placement.worldPosition, gridSize, mUGCEditLayer);
        if (isMouseDown && !mUGCContinuousPlacementLayer) {
            mUGCContinuousPlacementLayer = placementLayer;
        }
        placement.worldPosition.y = static_cast<float>(placementLayer) * gridSize;
    }

    glm::vec3 previewPosition = placement.worldPosition;
    if (mContext.game->GetIsUGCMode() && mPlacementOptions.showUGCPlatformPreview) {
        const float gridSize = mContext.game->GetUGCGridSize();
        glm::vec3 previewModelScale = mUGCPlacementPreviewModelScale;
        if (usesPlatformFootprint) {
            previewPosition = UGCPlatformGrid::CalculateFootprintPreviewPosition(placement.worldPosition, gridSize,
                                                                                 mUGCPlatformFootprintSideLength);
            previewModelScale =
                UGCPlatformGrid::CalculateFootprintPreviewScale(gridSize, mUGCPlatformFootprintSideLength);
        } else {
            previewPosition = UGCPlatformGrid::CalculateCellPreviewPosition(placement.worldPosition, gridSize);
            placement.worldPosition = previewPosition;
        }
        mContext.game->SetUGCPlatformPlacementPreview(previewPosition);
        if (mFixedPlacementPreviewPositions.empty()) {
            mContext.game->SetUGCPlacementModelPreview(previewPosition, mUGCPlacementPreviewModelPath,
                                                       previewModelScale, mUGCPlacementPreviewTextureOverridePath);
        } else {
            std::vector<glm::vec3> previewPositions = mFixedPlacementPreviewPositions;
            previewPositions.push_back(previewPosition);
            mContext.game->SetUGCPlacementModelPreviewPositions(previewPositions, mUGCPlacementPreviewModelPath,
                                                                previewModelScale,
                                                                mUGCPlacementPreviewTextureOverridePath);
        }
        if (mMovingPlatformPathStartPosition) {
            mContext.game->SetUGCMovingPlatformPathPreview(mMovingPlatformPathStartPosition, previewPosition);
        }
    }
    mPlacementPreviewPosition = previewPosition;
    const bool placementInputTriggered =
        mPlacementOptions.continuousPlacement ? isMouseDown : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (!placementInputTriggered) {
        return;
    }

    if (mPlacementOptions.continuousPlacement) {
        const float gridSize = mContext.game->GetUGCGridSize();
        const glm::ivec3 paintedCell = UGCPlatformGrid::CalculateGridPosition(placement.worldPosition, gridSize);
        if (mLastPlacementAttemptCell && *mLastPlacementAttemptCell == paintedCell) {
            return;
        }
        if (!mIsContinuousPlacementStrokeActive) {
            mIsContinuousPlacementStrokeActive = true;
        }
        mLastPlacementAttemptCell = paintedCell;
    }

    const auto create = [this, planetIndex, &placement]() { return mPlacementCreator(planetIndex, placement); };
    const StageEditGrouping grouping = mPlacementOptions.continuousPlacement && mHasSavedPlacementStroke
                                           ? StageEditGrouping::ContinueOperation
                                           : StageEditGrouping::NewOperation;
    const bool created = mExecuteEdit ? mExecuteEdit(create, grouping) : create();
    if (created && mPlacementOptions.continuousPlacement) {
        mHasSavedPlacementStroke = true;
    }
    if (created && mPlacementCompletedCallback) {
        mPlacementCompletedCallback();
    }
    if (mTargetPlatformSelector) {
        return;
    }
    mPlacementStatus = created ? mPlacementDisplayName + "を配置しました。続けてクリックできます"
                               : mPlacementDisplayName + "の配置に失敗しました";
}

std::vector<glm::vec3>
StageActorPlacementController::CalculateStrokePreviewPositions(const glm::ivec3& translationCells) const
{
    std::vector<glm::vec3> positions;
    if (!mContext.game) {
        return positions;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    for (const glm::ivec3& startCell : mMovingPlatformStartCells) {
        for (const glm::ivec3& footprintCell :
             UGCPlatformGrid::CalculateFootprintCells(startCell + translationCells, mUGCPlatformFootprintSideLength)) {
            positions.push_back(UGCPlatformGrid::CalculateCellWorldPosition(footprintCell, gridSize));
        }
    }
    return positions;
}

void StageActorPlacementController::UpdateMovingPlatformStrokePlacement()
{
    if (!mContext.game || !mSelectionController) {
        mPlacementStatus = "移動足場の配置に必要なシステムを利用できません";
        return;
    }

    mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
    mContext.game->SetUGCPlacementModelPreview(std::nullopt);
    mContext.game->SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);
    mPlacementPreviewPosition.reset();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        CancelPlacement();
        return;
    }
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    StageActorPlacement placement;
    if (!mPlacementResolver.TryResolveUGCBuildPlanePlacement(rayFrom, rayTo, mUGCEditLayer, placement)) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 hoveredFootprintCenter = UGCPlatformGrid::CalculateFootprintPreviewPosition(
        placement.worldPosition, gridSize, mUGCPlatformFootprintSideLength);
    mPlacementPreviewPosition = hoveredFootprintCenter;
    const glm::ivec3 hoveredCell = UGCPlatformGrid::CalculateGridPosition(placement.worldPosition, gridSize);
    const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (mMovingPlatformPlacementPhase != MovingPlatformPlacementPhase::Destination) {
        if (mMovingPlatformPlacementPhase == MovingPlatformPlacementPhase::WaitForSourceRelease) {
            if (isMouseDown) {
                return;
            }
            mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::SourceStroke;
        }

        if (isMouseDown && std::find(mMovingPlatformStartCells.begin(), mMovingPlatformStartCells.end(), hoveredCell) ==
                               mMovingPlatformStartCells.end()) {
            mMovingPlatformStartCells.push_back(hoveredCell);
            mWasMovingPlatformStrokeActive = true;
        }

        if (mMovingPlatformStartCells.empty()) {
            mContext.game->SetUGCPlatformPlacementPreview(hoveredFootprintCenter);
            mContext.game->SetUGCPlacementModelPreview(
                hoveredFootprintCenter, mUGCPlacementPreviewModelPath,
                UGCPlatformGrid::CalculateFootprintPreviewScale(gridSize, mUGCPlatformFootprintSideLength),
                mUGCPlacementPreviewTextureOverridePath);
            return;
        }

        const std::vector<glm::vec3> sourcePreviewPositions = CalculateStrokePreviewPositions(glm::ivec3(0));
        mContext.game->SetUGCPlatformPlacementPreview(hoveredFootprintCenter);
        mContext.game->SetUGCPlacementModelPreviewPositions(
            sourcePreviewPositions, mUGCPlacementPreviewModelPath,
            glm::vec3(gridSize * 0.5f, gridSize * 0.1f, gridSize * 0.5f), mUGCPlacementPreviewTextureOverridePath);

        if (!isMouseDown && mWasMovingPlatformStrokeActive) {
            mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::Destination;
            mPlacementDisplayName = "移動足場：移動先を配置";
            mPlacementStatus = "移動先をクリックして配置してください";
        }
        return;
    }

    const glm::ivec3 destinationOffset = hoveredCell - mMovingPlatformStartCells.front();
    const std::vector<glm::vec3> sourcePreviewPositions = CalculateStrokePreviewPositions(glm::ivec3(0));
    const std::vector<glm::vec3> destinationPreviewPositions = CalculateStrokePreviewPositions(destinationOffset);
    std::vector<glm::vec3> movementPreviewPositions = sourcePreviewPositions;
    movementPreviewPositions.insert(movementPreviewPositions.end(), destinationPreviewPositions.begin(),
                                    destinationPreviewPositions.end());
    mContext.game->SetUGCPlatformPlacementPreview(hoveredFootprintCenter);
    mContext.game->SetUGCPlacementModelPreviewPositions(movementPreviewPositions, mUGCPlacementPreviewModelPath,
                                                        glm::vec3(gridSize * 0.5f, gridSize * 0.1f, gridSize * 0.5f),
                                                        mUGCPlacementPreviewTextureOverridePath);

    const glm::vec3 startCenter = sourcePreviewPositions.front();
    mContext.game->SetUGCMovingPlatformPathPreview(startCenter, destinationPreviewPositions.front());
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }
    const auto create = [this, &destinationOffset]() {
        return mMovingPlatformCreator(0, mMovingPlatformStartCells, destinationOffset);
    };
    const bool created = mExecuteEdit ? mExecuteEdit(create, StageEditGrouping::NewOperation) : create();
    if (!created) {
        mPlacementStatus = "移動足場を配置できませんでした";
        return;
    }

    PrepareNextMovingPlatformPlacement();
}

void StageActorPlacementController::PrepareNextMovingPlatformPlacement()
{
    mMovingPlatformStartCells.clear();
    mWasMovingPlatformStrokeActive = false;
    mMovingPlatformPlacementPhase = MovingPlatformPlacementPhase::WaitForSourceRelease;
    mPlacementPreviewPosition.reset();
    mMovingPlatformPathStartPosition.reset();
    mFixedPlacementPreviewPositions.clear();
    mPlacementDisplayName = "移動足場：出発地点を長押しで配置";
    mPlacementStatus = "移動足場を配置しました。次の出発側を長押ししてください";

    if (!mContext.game) {
        return;
    }
    mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
    mContext.game->SetUGCPlacementModelPreview(std::nullopt);
    mContext.game->SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);
}

void StageActorPlacementController::UpdateTargetPlatformSelection()
{
    if (!mContext.game || !mSelectionController) {
        mPlacementStatus = "足場の選択に必要なシステムを利用できません";
        return;
    }

    mContext.game->SetUGCPlatformPlacementPreview(std::nullopt);
    mContext.game->SetUGCPlacementModelPreview(std::nullopt);
    mContext.game->SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);

    if (!mFixedPlacementPreviewPositions.empty()) {
        mContext.game->SetUGCPlatformPlacementPreview(mFixedPlacementPreviewPositions.front());
        mContext.game->SetUGCPlacementModelPreviewPositions(
            mFixedPlacementPreviewPositions, mUGCPlacementPreviewModelPath, mUGCPlacementPreviewModelScale,
            mUGCPlacementPreviewTextureOverridePath);
    } else if (mTargetSelectionSourcePreviewPosition) {
        mContext.game->SetUGCPlatformPlacementPreview(mTargetSelectionSourcePreviewPosition);
        mContext.game->SetUGCPlacementModelPreview(mTargetSelectionSourcePreviewPosition, mUGCPlacementPreviewModelPath,
                                                   mUGCPlacementPreviewModelScale,
                                                   mUGCPlacementPreviewTextureOverridePath);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        CancelPlacement();
        return;
    }

    if (ImGui::GetIO().WantCaptureMouse || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    mSelectionController->Update();
    mSelectionController->ConsumeRequestOpenPlacement();
    Platform* targetPlatform = dynamic_cast<Platform*>(mSelectionController->GetPickedActor());
    const bool isSwitchPlatform = targetPlatform && (targetPlatform->GetPressureSwitchComponent() ||
                                                     targetPlatform->GetLatchedGroupSwitchComponent());
    if (!targetPlatform || isSwitchPlatform) {
        const bool created =
            mPlacementWithoutTargetCreator &&
            (mExecuteEdit ? mExecuteEdit(mPlacementWithoutTargetCreator, StageEditGrouping::NewOperation)
                          : mPlacementWithoutTargetCreator());
        if (!created) {
            mPlacementStatus = "スイッチを配置できませんでした";
            return;
        }

        std::function<void()> continuePlacementCallback = std::move(mContinuePlacementCallback);
        mSelectionController->Clear();
        CancelPlacement();
        if (continuePlacementCallback) {
            continuePlacementCallback();
        }
        return;
    }

    const auto create = [this, targetPlatform]() { return mTargetPlatformSelector(targetPlatform); };
    if (!(mExecuteEdit ? mExecuteEdit(create, StageEditGrouping::NewOperation) : create())) {
        mPlacementStatus = "選んだ足場をスイッチに設定できませんでした";
        return;
    }

    std::function<void()> continuePlacementCallback = std::move(mContinuePlacementCallback);
    mSelectionController->Clear();
    CancelPlacement();
    if (continuePlacementCallback) {
        continuePlacementCallback();
    }
}
