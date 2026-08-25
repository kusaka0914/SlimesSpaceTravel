#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/enemy/EnemyPresetRepository.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
std::string ToLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

StageActorPlacement CreateUGCWorldUpPlacement(
    const StageActorPlacement& surfacePlacement)
{
    StageActorPlacement worldUpPlacement = surfacePlacement;
    worldUpPlacement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    return worldUpPlacement;
}

glm::vec3 CreateUGCPlatformScale(float gridSize, int footprintSideLength)
{
    const float sideLength = static_cast<float>(
        std::clamp(footprintSideLength, 1, 3));
    return glm::vec3(
        sideLength * gridSize * 0.5f,
        0.1f * gridSize,
        sideLength * gridSize * 0.5f);
}

}

StageAddActorPanel::StageAddActorPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mCreateService(context)
{
    std::snprintf(mNPCName.data(), mNPCName.size(), "%s", "新しいNPC");
    mNPCTalkTexts.emplace_back();
    std::snprintf(
        mNPCTalkTexts.front().data(),
        mNPCTalkTexts.front().size(),
        "%s",
        "こんにちは");
    mTutorialTriggerTalkTexts.emplace_back();
    std::snprintf(
        mTutorialTriggerTalkTexts.front().data(),
        mTutorialTriggerTalkTexts.front().size(),
        "%s",
        "ここにチュートリアルの内容を入力");
}

void StageAddActorPanel::SetSelectionController(
    StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void StageAddActorPanel::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mPushUndoCallback = std::move(pushUndoCallback);
}

bool StageAddActorPanel::ActivateUGCPreset(UGCPresetKind presetKind)
{
    switch (presetKind) {
    case UGCPresetKind::NormalPlatform:
        if (!mCreateService.RefreshUGCPlatformCells()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        BeginPlacement(
            "通常足場",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                const StageActorPlacement worldUpPlacement =
                    CreateUGCWorldUpPlacement(placement);
                const float gridSize =
                    mContext.game->GetUGCGridSize();
                const bool added = mCreateService.AddUGCPlatformCell(
                    planetIndex,
                    worldUpPlacement,
                    gridSize,
                    mUGCPlatformFootprintSideLength);
                if (added && mSelectionController) {
                    mSelectionController->Clear();
                }
                return added;
            },
            true,
            true,
            false,
            true);
        mUGCPlacementPreviewModelPath = "platform.obj";
        mUGCPlacementPreviewModelScale = glm::vec3(1.0f);
        return true;
    case UGCPresetKind::NormalEnemy:
        BeginPlacement(
            "通常敵",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const StageActorPlacement worldUpPlacement =
                    CreateUGCWorldUpPlacement(placement);
                return mCreateService.AddEnemy(
                    "normal",
                    planetIndex,
                    &worldUpPlacement);
            });
        mUGCPlacementPreviewModelPath = "enemy.obj";
        mUGCPlacementPreviewModelScale = glm::vec3(0.25f);
        return true;
    case UGCPresetKind::EllipsePlanet:
        CancelPlacement();
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }
        return mCreateService.AddEllipsePlanet("planet.obj");
    case UGCPresetKind::PressureSwitch:
        BeginPlacement(
            "スイッチ",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const StageActorPlacement worldUpPlacement =
                    CreateUGCWorldUpPlacement(placement);
                return mCreateService.AddPressureSwitchPlatform(
                    planetIndex,
                    "platform.obj",
                    glm::vec3(0.75f, 0.2f, 0.75f),
                    &worldUpPlacement);
            });
        mUGCPlacementPreviewModelPath = "platform.obj";
        mUGCPlacementPreviewModelScale = glm::vec3(0.75f, 0.2f, 0.75f);
        return true;
    case UGCPresetKind::GoalStar:
        BeginPlacement(
            "ゴールの星",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                return mCreateService.AddStar(planetIndex, &placement);
            });
        mUGCPlacementPreviewModelPath = "star.obj";
        mUGCPlacementPreviewModelScale = glm::vec3(0.3f);
        return true;
    case UGCPresetKind::MovingPlatform:
    {
        if (!mCreateService.RefreshUGCPlatformCells()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        auto startPlacement = std::make_shared<std::optional<StageActorPlacement>>();
        BeginPlacement(
            "移動足場：出発点",
            0,
            [this, startPlacement](int planetIndex, const StageActorPlacement& placement) {
                if (!*startPlacement) {
                    const float gridSize = mContext.game->GetUGCGridSize();
                    *startPlacement = CreateUGCWorldUpPlacement(placement);
                    mPlacementDisplayName = "移動足場：到着点";
                    mPlacementStatus = "到着点をクリックしてください";
                    return true;
                }
                if (mPushUndoCallback) mPushUndoCallback();
                const float gridSize = mContext.game->GetUGCGridSize();
                const glm::ivec3 startCell(
                    static_cast<int>(std::floor(
                        (*startPlacement)->worldPosition.x / gridSize)),
                    static_cast<int>(std::round(
                        (*startPlacement)->worldPosition.y / gridSize)),
                    static_cast<int>(std::floor(
                        (*startPlacement)->worldPosition.z / gridSize)));
                const glm::ivec3 endCell(
                    static_cast<int>(std::floor(placement.worldPosition.x / gridSize)),
                    static_cast<int>(std::round(placement.worldPosition.y / gridSize)),
                    static_cast<int>(std::floor(placement.worldPosition.z / gridSize)));
                const bool created = mCreateService.AddUGCPlatformCell(
                    planetIndex, **startPlacement, gridSize,
                    mUGCPlatformFootprintSideLength, "moving",
                    endCell - startCell);
                if (created) {
                    *startPlacement = std::nullopt;
                    mPlacementDisplayName = "移動足場：出発点";
                    if (mSelectionController) {
                        mSelectionController->Clear();
                    }
                }
                return created;
            });
        mUGCPlacementPreviewModelPath = "platform.obj";
        mUGCPlacementPreviewModelScale = CreateUGCPlatformScale(
            mContext.game->GetUGCGridSize(),
            mUGCPlatformFootprintSideLength);
        return true;
    }
    case UGCPresetKind::FadingPlatform:
    case UGCPresetKind::AdhesivePlatform:
    {
        if (!mCreateService.RefreshUGCPlatformCells()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        const bool fading = presetKind == UGCPresetKind::FadingPlatform;
        BeginPlacement(
            fading ? "消える足場" : "くっつき足場",
            0,
            [this, fading](int planetIndex, const StageActorPlacement& placement) {
                const float gridSize = mContext.game->GetUGCGridSize();
                const bool added = mCreateService.AddUGCPlatformCell(
                    planetIndex, CreateUGCWorldUpPlacement(placement),
                    gridSize, mUGCPlatformFootprintSideLength,
                    fading ? "fading" : "adhesive");
                if (added && mSelectionController) {
                    mSelectionController->Clear();
                }
                return added;
            }, true, true, false, true);
        mUGCPlacementPreviewModelPath = "platform.obj";
        mUGCPlacementPreviewModelScale = CreateUGCPlatformScale(
            mContext.game->GetUGCGridSize(),
            mUGCPlatformFootprintSideLength);
        return true;
    }
    case UGCPresetKind::TwoPlayerSwitch:
    {
        auto firstPlacement = std::make_shared<std::optional<StageActorPlacement>>();
        BeginPlacement(
            "2人用スイッチ：1つ目",
            0,
            [this, firstPlacement](int planetIndex, const StageActorPlacement& placement) {
                if (!*firstPlacement) {
                    *firstPlacement = CreateUGCWorldUpPlacement(placement);
                    mPlacementDisplayName = "2人用スイッチ：2つ目";
                    mPlacementStatus = "もう1つのスイッチをクリックしてください";
                    return true;
                }
                if (mPushUndoCallback) mPushUndoCallback();
                const bool created = mCreateService.AddTwoPlayerSwitchPair(
                    planetIndex, **firstPlacement,
                    CreateUGCWorldUpPlacement(placement));
                if (created) *firstPlacement = std::nullopt;
                return created;
            });
        mUGCPlacementPreviewModelPath = "platform.obj";
        mUGCPlacementPreviewModelScale = glm::vec3(0.75f, 0.2f, 0.75f);
        return true;
    }
    }

    return false;
}

bool StageAddActorPanel::TryEraseUGCPlatformCell()
{
    if (!mSelectionController ||
        !mContext.game ||
        !mContext.game->GetCurrentStage() ||
        !mContext.game->GetPhysicsSystem()) {
        return false;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return false;
    }

    StageActorPlacement placement;
    if (!TryCreateUGCFallbackPlacement(rayFrom, rayTo, placement)) {
        return false;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        mLastErasedUGCCell.reset();
        return false;
    }
    const float gridSize = mContext.game->GetUGCGridSize();
    int eraseLayer = mUGCEditLayer;
    if (!mCreateService.ResolveUGCPlatformLayerAtGridPosition(
            0,
            placement.worldPosition,
            gridSize,
            mUGCEditLayer,
            eraseLayer)) {
        return false;
    }

    const glm::ivec3 erasedCell(
        static_cast<int>(std::floor(placement.worldPosition.x / gridSize)),
        eraseLayer,
        static_cast<int>(std::floor(placement.worldPosition.z / gridSize)));
    if (mLastErasedUGCCell && *mLastErasedUGCCell == erasedCell) {
        return false;
    }

    if (mPushUndoCallback) {
        mPushUndoCallback();
    }
    const bool removed = mCreateService.RemoveUGCPlatformCellAtGridPosition(
        0,
        placement.worldPosition,
        gridSize,
        eraseLayer);
    if (removed) {
        mLastErasedUGCCell = erasedCell;
        mSelectionController->Clear();
    }
    return removed;
}

bool StageAddActorPanel::TryTranslateUGCPlatformCells(
    const StageActorRef& actorRef,
    const glm::vec3& worldDelta)
{
    return mCreateService.TranslateUGCPlatformCells(
        actorRef, worldDelta);
}

bool StageAddActorPanel::TryTranslateUGCPlatformCells(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mCreateService.TranslateUGCPlatformCells(
        actorRefs, worldDelta);
}

bool StageAddActorPanel::BeginDuplicatePlacement(
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
        ResolveHitPlanetIndex(sourceActor, 0);
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

void StageAddActorPanel::BeginPlacement(
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

void StageAddActorPanel::CancelPlacement()
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

int StageAddActorPanel::ResolveHitPlanetIndex(
    Actor* hitActor,
    int fallbackPlanetIndex) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return fallbackPlanetIndex;
    }

    Planet* hitPlanet = dynamic_cast<Planet*>(hitActor);
    if (!hitPlanet && hitActor) {
        hitPlanet = hitActor->GetCurrentPlanet();
    }
    if (!hitPlanet) {
        return fallbackPlanetIndex;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const auto planetIt = std::find(planets.begin(), planets.end(), hitPlanet);
    if (planetIt == planets.end()) {
        return fallbackPlanetIndex;
    }
    return static_cast<int>(std::distance(planets.begin(), planetIt));
}

bool StageAddActorPanel::TryCreateUGCFallbackPlacement(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo,
    StageActorPlacement& outPlacement) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty() || !planets.front()) {
        return false;
    }

    const glm::vec3 rayDelta = rayTo - rayFrom;
    constexpr float parallelEpsilon = 0.00001f;
    float rayParameter = -1.0f;

    const float buildPlaneY =
        static_cast<float>(mUGCEditLayer) *
        mContext.game->GetUGCGridSize();


    // UGCの組立面は、重なった足場ではなく選択中レイヤーの床に固定する。
    if (std::abs(rayDelta.y) > parallelEpsilon) {
        rayParameter =
            (buildPlaneY - rayFrom.y) / rayDelta.y;
    }

    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        // 横向きの視線は組立面と平行になり得るため、同じ惑星中心を通るカメラ正面の面で配置位置を決める。



        const glm::vec3 buildPlaneCenter(
            planets.front()->GetPos().x,
            buildPlaneY,
            planets.front()->GetPos().z);
        glm::vec3 viewPlaneNormal = rayFrom - buildPlaneCenter;
        const float normalLength = glm::length(viewPlaneNormal);
        if (normalLength <= parallelEpsilon) {
            return false;
        }
        viewPlaneNormal /= normalLength;

        const float denominator = glm::dot(rayDelta, viewPlaneNormal);
        if (std::abs(denominator) <= parallelEpsilon) {
            return false;
        }
        rayParameter = glm::dot(
            buildPlaneCenter - rayFrom,
            viewPlaneNormal) / denominator;
    }

    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        return false;
    }

    outPlacement.worldPosition = rayFrom + rayDelta * rayParameter;
    outPlacement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    return true;
}

void StageAddActorPanel::UpdatePlacement()
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
        if (!TryCreateUGCFallbackPlacement(rayFrom, rayTo, placement)) {
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
        planetIndex = ResolveHitPlanetIndex(
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
        placement.worldPosition.x =
            std::round(placement.worldPosition.x / gridSize) * gridSize;
        placement.worldPosition.y =
            static_cast<float>(mUGCEditLayer) * gridSize;
        placement.worldPosition.z =
            std::round(placement.worldPosition.z / gridSize) * gridSize;
    }

    if (mContext.game->GetIsUGCMode() && mAutoStackUGCPlatforms) {
        const float gridSize = mContext.game->GetUGCGridSize();
        const int placementLayer = mUGCContinuousPlacementLayer
            ? *mUGCContinuousPlacementLayer
            : mCreateService.ResolveUGCPlatformPlacementLayerAtGridPosition(
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
            const float footprintSideLength =
                static_cast<float>(mUGCPlatformFootprintSideLength);


            previewPosition.x =
                (std::floor(placement.worldPosition.x / gridSize) +
                 1.0f - footprintSideLength * 0.5f) * gridSize;
            previewPosition.z =
                (std::floor(placement.worldPosition.z / gridSize) +
                 1.0f - footprintSideLength * 0.5f) * gridSize;
            previewModelScale = glm::vec3(
                footprintSideLength * gridSize * 0.5f,
                0.1f * gridSize,
                footprintSideLength * gridSize * 0.5f);
        } else {
            previewPosition.x =
                (std::floor(placement.worldPosition.x / gridSize) + 0.5f) *
                gridSize;
            previewPosition.z =
                (std::floor(placement.worldPosition.z / gridSize) + 0.5f) *
                gridSize;





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
        const glm::ivec3 paintedCell(
            static_cast<int>(std::floor(
                placement.worldPosition.x / gridSize)),
            static_cast<int>(std::round(
                placement.worldPosition.y / gridSize)),
            static_cast<int>(std::floor(
                placement.worldPosition.z / gridSize)));
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

    DrawBoatArrivalPointCreation();
    DrawJewelItemCreation();
    DrawHazardActorCreation();

    if (mPlacementCreator) {
        ImGui::SeparatorText("連続配置中");
        ImGui::Text("配置対象: %s", mPlacementDisplayName.c_str());
        ImGui::TextWrapped("ゲーム画面をクリックするたびに追加します。");
        if (ImGui::Button("追加解除") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelPlacement();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("ESCでも解除");
        if (!mPlacementStatus.empty()) {
            ImGui::TextWrapped("%s", mPlacementStatus.c_str());
        }
        ImGui::Separator();
    }

    if (ImGui::TreeNode("汎用モデル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
        if (planets.empty()) {
            ImGui::TextUnformatted("惑星が存在しないため、モデルを追加できません");
        } else {
            DrawPlanetCombo("追加先の惑星##stageObject", mSelectedStageObjectPlanetIndex);
            ImGui::InputTextWithHint(
                "##stageObjectSearch",
                "モデル名を検索",
                mStageObjectSearch.data(),
                mStageObjectSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText = ToLower(mStageObjectSearch.data());

            ImGui::BeginChild("StageObjectAssetPicker", ImVec2(0.0f, 180.0f), true);
            for (const std::string& modelPath : modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) == std::string::npos) {
                    continue;
                }

                const bool selected = modelPath == mSelectedStageObjectModel;
                if (ImGui::Selectable(modelPath.c_str(), selected)) {
                    mSelectedStageObjectModel = modelPath;
                }
            }
            ImGui::EndChild();

            ImGui::Button(
                "モデルアセットをここへドロップ##newStageObjectModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedStageObjectModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedStageObjectModelPath)) {
                mSelectedStageObjectModel =
                    droppedStageObjectModelPath;
            }

            ImGui::Text(
                "選択中: %s",
                mSelectedStageObjectModel.empty()
                    ? "未選択"
                    : mSelectedStageObjectModel.c_str());
            ImGui::Checkbox("モデル形状の当たり判定を作る", &mStageObjectCollisionEnabled);

            const bool canAdd =
                mSelectedStageObjectPlanetIndex >= 0 &&
                !mSelectedStageObjectModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("選択したモデルをステージに追加")) {
                const std::string modelPath = mSelectedStageObjectModel;
                const bool collisionEnabled = mStageObjectCollisionEnabled;
                BeginPlacement(
                    "ステージモデル",
                    mSelectedStageObjectPlanetIndex,
                    [this, modelPath, collisionEnabled](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddStageObject(
                            planetIndex, modelPath, collisionEnabled, &placement);
                    });
                mStageObjectStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mStageObjectStatus.empty()) {
                ImGui::SameLine();
                ImGui::TextUnformatted(mStageObjectStatus.c_str());
            }

            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("惑星追加")) {
        const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
        const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

        if (ImGui::Combo(
                "惑星モデル",
                &mSelectedPlanetModelIndex,
                planetModelLabels,
                IM_ARRAYSIZE(planetModelLabels))) {
            mSelectedPlanetModelPath =
                planetModels[mSelectedPlanetModelIndex];
        }

        ImGui::Button(
            "モデルアセットをここへドロップ##newPlanetModel",
            ImVec2(-1.0f, 0.0f));
        std::string droppedPlanetModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedPlanetModelPath)) {
            mSelectedPlanetModelPath = droppedPlanetModelPath;
        }
        ImGui::TextWrapped(
            "選択中: %s",
            mSelectedPlanetModelPath.c_str());

        if (ImGui::Button("惑星を追加")) {
            mCreateService.AddPlanet(mSelectedPlanetModelPath);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("敵追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、敵を追加できません");
            ImGui::TreePop();
            return;
        }

        DrawPlanetCombo("敵の追加先惑星", mSelectedEnemyPlanetIndex);

        const std::uint64_t currentPresetRevision =
            EnemyPresetRepository::GetRevision();
        if (!mEnemyPresetsLoaded ||
            mLoadedEnemyPresetRevision != currentPresetRevision) {
            mEnemyPresetsLoaded = true;
            mLoadedEnemyPresetRevision = currentPresetRevision;
            EnemyPresetRepository::Load(
                "../assets/data/actor/enemies.yaml",
                mEnemyPresets,
                mEnemyPresetLoadError);
        }
        if (mEnemyPresets.empty()) {
            ImGui::TextWrapped(
                "敵プリセットを読み込めません: %s",
                mEnemyPresetLoadError.c_str());
            ImGui::TreePop();
            return;
        }

        mSelectedEnemyTypeIndex = std::clamp(
            mSelectedEnemyTypeIndex,
            0,
            static_cast<int>(mEnemyPresets.size()) - 1);
        const EnemyPresetDefinition& selectedEnemyPreset =
            mEnemyPresets[mSelectedEnemyTypeIndex];
        if (ImGui::BeginCombo(
                "敵プリセット",
                selectedEnemyPreset.displayName.c_str())) {
            for (std::size_t presetIndex = 0;
                 presetIndex < mEnemyPresets.size();
                 ++presetIndex) {
                const bool isSelected =
                    static_cast<int>(presetIndex) ==
                    mSelectedEnemyTypeIndex;
                const std::string label =
                    mEnemyPresets[presetIndex].displayName +
                    " (" + mEnemyPresets[presetIndex].id + ")";
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    mSelectedEnemyTypeIndex =
                        static_cast<int>(presetIndex);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        const bool canAddEnemy = mSelectedEnemyPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            const std::string enemyType =
                mEnemyPresets[mSelectedEnemyTypeIndex].id;
            BeginPlacement(
                "敵",
                mSelectedEnemyPlanetIndex,
                [this, enemyType](int planetIndex, const StageActorPlacement& placement) {
                    return mCreateService.AddEnemy(enemyType, planetIndex, &placement);
                });
        }

        if (!canAddEnemy) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("足場追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、足場を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("追加先の惑星##platform", mSelectedPlatformPlanetIndex);

            const char* platformModelLabels[] = {"通常足場", "カーブ足場", "細い足場"};
            const char* platformModels[] = {"platform.obj", "curvePlatform.obj", "platform_thin.obj"};

            if (ImGui::Combo(
                    "モデル##platform",
                    &mSelectedPlatformModelIndex,
                    platformModelLabels,
                    IM_ARRAYSIZE(platformModelLabels))) {
                mSelectedPlatformModelPath =
                    platformModels[mSelectedPlatformModelIndex];
            }
            ImGui::Button(
                "モデルアセットをここへドロップ##newPlatformModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedPlatformModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedPlatformModelPath)) {
                mSelectedPlatformModelPath =
                    droppedPlatformModelPath;
            }
            ImGui::TextWrapped(
                "選択中: %s",
                mSelectedPlatformModelPath.c_str());

            ImGui::SliderFloat("スケールX##platform", &mPlatformScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &mPlatformScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &mPlatformScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = mSelectedPlatformPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                const std::string modelPath = mSelectedPlatformModelPath;
                const glm::vec3 scale = mPlatformScale;
                BeginPlacement(
                    "足場",
                    mSelectedPlatformPlanetIndex,
                    [this, modelPath, scale](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddPlatform(planetIndex, modelPath, scale, &placement);
                    });
            }

            if (ImGui::Button("乗ると動く足場を追加")) {
                const std::string modelPath = mSelectedPlatformModelPath;
                const glm::vec3 scale = mPlatformScale;
                BeginPlacement(
                    "乗ると動く足場",
                    mSelectedPlatformPlanetIndex,
                    [this, modelPath, scale](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddRideMovingPlatform(
                            planetIndex, modelPath, scale, &placement);
                    });
                mRideMovingPlatformStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAddPlatform) {
                ImGui::EndDisabled();
            }

            if (!mRideMovingPlatformStatus.empty()) {
                ImGui::TextUnformatted(mRideMovingPlatformStatus.c_str());
            }
            ImGui::TextDisabled(
                "追加後は「配置」から出発地点と到着地点を調整できます。");

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("クリスタル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、クリスタルを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("クリスタルの追加先惑星", mSelectedCrystalPlanetIndex);

            const char* crystalTypeLabels[] = {"小さいクリスタル", "大きいクリスタル"};
            const char* crystalTypes[] = {"little", "big"};

            ImGui::Combo("クリスタルタイプ", &mSelectedCrystalTypeIndex, crystalTypeLabels,
                         IM_ARRAYSIZE(crystalTypeLabels));

            const bool canAddCrystal = mSelectedCrystalPlanetIndex >= 0;

            if (!canAddCrystal) {
                ImGui::Text("クリスタルを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("クリスタルを追加")) {
                const std::string crystalType = crystalTypes[mSelectedCrystalTypeIndex];
                BeginPlacement(
                    "クリスタル",
                    mSelectedCrystalPlanetIndex,
                    [this, crystalType](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddCrystal(crystalType, planetIndex, &placement);
                    });
            }

            if (!canAddCrystal) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("NPC追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、NPCを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("NPCの追加先惑星", mSelectedNPCPlanetIndex);

            ImGui::InputTextWithHint(
                "##npcModelSearch",
                "NPCモデル名を検索",
                mNPCModelSearch.data(),
                mNPCModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText = ToLower(mNPCModelSearch.data());

            ImGui::BeginChild("NPCModelAssetPicker", ImVec2(0.0f, 180.0f), true);
            for (const std::string& modelPath : modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) == std::string::npos) {
                    continue;
                }

                const bool selected = modelPath == mSelectedNPCModel;
                if (ImGui::Selectable(modelPath.c_str(), selected)) {
                    mSelectedNPCModel = modelPath;
                }
            }
            ImGui::EndChild();

            ImGui::Button(
                "モデルアセットをここへドロップ##newNPCModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedNPCModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedNPCModelPath)) {
                mSelectedNPCModel = droppedNPCModelPath;
            }

            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedNPCModel.empty() ? "未選択" : mSelectedNPCModel.c_str());
            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");

            ImGui::InputText("NPC名", mNPCName.data(), mNPCName.size());
            ImGui::DragFloat(
                "初期スケール",
                &mNPCScale,
                0.01f,
                0.01f,
                30.0f,
                "%.2f");
            ImGui::DragFloat(
                "会話できる距離",
                &mNPCTalkRadius,
                0.05f,
                0.1f,
                20.0f,
                "%.2f");

            ImGui::SeparatorText("会話内容");
            for (std::size_t talkIndex = 0;
                 talkIndex < mNPCTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "会話 " + std::to_string(talkIndex + 1) +
                    "##newNPCTalk" + std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mNPCTalkTexts[talkIndex].data(),
                    mNPCTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mNPCTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("この会話を削除##newNPCTalkDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mNPCTalkTexts.erase(
                        mNPCTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(talkIndex));
                    break;
                }
            }

            if (ImGui::Button("会話を追加##newNPC")) {
                mNPCTalkTexts.emplace_back();
            }

            const bool canAddNPC =
                mSelectedNPCPlanetIndex >= 0 &&
                !mSelectedNPCModel.empty();

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星とモデルを選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(mNPCTalkTexts.size());
                for (const auto& talkText : mNPCTalkTexts) {
                    talkTexts.emplace_back(talkText.data());
                }

                const std::string modelPath = mSelectedNPCModel;
                const std::string name = mNPCName.data();
                const float talkRadius = mNPCTalkRadius;
                const float scale = mNPCScale;
                BeginPlacement(
                    "NPC",
                    mSelectedNPCPlanetIndex,
                    [this, modelPath, name, talkTexts, talkRadius, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddNPC(
                            modelPath, planetIndex, name, talkTexts, talkRadius, scale, &placement);
                    });
                mNPCStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            if (!mNPCStatus.empty()) {
                ImGui::TextUnformatted(mNPCStatus.c_str());
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("チュートリアルトリガー追加")) {
        const auto& planets =
            mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::TextUnformatted(
                "追加先の惑星がありません");
        } else {
            DrawPlanetCombo(
                "追加先の惑星##tutorialTrigger",
                mSelectedTutorialTriggerPlanetIndex);

            ImGui::InputTextWithHint(
                "##tutorialTriggerModelSearch",
                "箱型モデルを検索",
                mTutorialTriggerModelSearch.data(),
                mTutorialTriggerModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText =
                ToLower(
                    mTutorialTriggerModelSearch.data());
            ImGui::BeginChild(
                "TutorialTriggerModelAssetPicker",
                ImVec2(0.0f, 180.0f),
                true);
            for (const std::string& modelPath :
                 modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) ==
                        std::string::npos) {
                    continue;
                }

                const bool selected =
                    modelPath ==
                    mSelectedTutorialTriggerModel;
                if (ImGui::Selectable(
                        modelPath.c_str(),
                        selected)) {
                    mSelectedTutorialTriggerModel =
                        modelPath;
                }
            }
            ImGui::EndChild();
            ImGui::Button(
                "モデルアセットをここへドロップ##newTutorialTriggerModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedTutorialTriggerModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedTutorialTriggerModelPath)) {
                mSelectedTutorialTriggerModel =
                    droppedTutorialTriggerModelPath;
            }
            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedTutorialTriggerModel.empty()
                    ? "未選択"
                    : mSelectedTutorialTriggerModel.c_str());

            ImGui::DragFloat3(
                "初期スケール##tutorialTrigger",
                &mTutorialTriggerScale.x,
                0.05f,
                0.01f,
                100.0f,
                "%.2f");

            ImGui::SeparatorText("チュートリアル内容");
            for (std::size_t talkIndex = 0;
                 talkIndex <
                 mTutorialTriggerTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "ページ " +
                    std::to_string(talkIndex + 1) +
                    "##newTutorialTriggerTalk" +
                    std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mTutorialTriggerTalkTexts[talkIndex].data(),
                    mTutorialTriggerTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mTutorialTriggerTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("このページを削除##newTutorialTriggerDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mTutorialTriggerTalkTexts.erase(
                        mTutorialTriggerTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(
                            talkIndex));
                    break;
                }
            }

            if (ImGui::Button(
                    "ページを追加##newTutorialTrigger")) {
                mTutorialTriggerTalkTexts.emplace_back();
            }

            const bool canAdd =
                mSelectedTutorialTriggerPlanetIndex >= 0 &&
                !mSelectedTutorialTriggerModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(
                    "チュートリアルトリガーを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(
                    mTutorialTriggerTalkTexts.size());
                for (const auto& talkText :
                     mTutorialTriggerTalkTexts) {
                    talkTexts.emplace_back(
                        talkText.data());
                }

                const std::string modelPath = mSelectedTutorialTriggerModel;
                const glm::vec3 scale = mTutorialTriggerScale;
                BeginPlacement(
                    "チュートリアルトリガー",
                    mSelectedTutorialTriggerPlanetIndex,
                    [this, modelPath, talkTexts, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddTutorialTrigger(
                            planetIndex, modelPath, talkTexts, scale, &placement);
                    });
                mTutorialTriggerStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mTutorialTriggerStatus.empty()) {
                ImGui::TextUnformatted(
                    mTutorialTriggerStatus.c_str());
            }
            ImGui::TextDisabled(
                "箱型モデルを使うと、モデルの位置・回転・スケールと反応範囲が一致します。");
            ImGui::TextDisabled(
                "ゲーム中は見えず、衝突しません。内部に入ると一度だけ開始します。");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("ボートパーツ追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートパーツを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートパーツの追加先惑星", mSelectedBoatPartsPlanetIndex);

            const char* boatPartsTypeLabels[] = {"パーツ1", "パーツ2", "パーツ3", "パーツ4", "パーツ5"};
            const char* boatPartsTypes[] = {"parts1", "parts2", "parts3", "parts4", "parts5"};

            ImGui::Combo("ボートパーツタイプ", &mSelectedBoatPartsTypeIndex, boatPartsTypeLabels,
                         IM_ARRAYSIZE(boatPartsTypeLabels));

            const bool canAddBoatParts = mSelectedBoatPartsPlanetIndex >= 0;

            if (!canAddBoatParts) {
                ImGui::Text("ボートパーツを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートパーツを追加")) {
                const std::string boatPartsType = boatPartsTypes[mSelectedBoatPartsTypeIndex];
                BeginPlacement(
                    "ボートパーツ",
                    mSelectedBoatPartsPlanetIndex,
                    [this, boatPartsType](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddBoatParts(boatPartsType, planetIndex, &placement);
                    });
            }

            if (!canAddBoatParts) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボート追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートの開始惑星", mSelectedBoatStartPlanetIndex);
            DrawPlanetCombo("ボートの移動先惑星", mSelectedBoatDestPlanetIndex);

            ImGui::InputInt("移動先ステージ", &mSelectedBoatDestStage);

            const bool canAddBoat = mSelectedBoatStartPlanetIndex >= 0 && mSelectedBoatDestPlanetIndex >= 0;

            if (!canAddBoat) {
                ImGui::Text("ボートを追加するには、開始惑星と移動先惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートを追加")) {
                const int destinationPlanetIndex = mSelectedBoatDestPlanetIndex;
                const int destinationStage = mSelectedBoatDestStage;
                BeginPlacement(
                    "ボート",
                    mSelectedBoatStartPlanetIndex,
                    [this, destinationPlanetIndex, destinationStage](
                        int startPlanetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddBoat(
                            startPlanetIndex, destinationPlanetIndex, destinationStage, &placement);
                    });
            }

            if (!canAddBoat) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("星追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、星を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("星の追加先惑星", mSelectedStarPlanetIndex);

            const bool canAddStar = mSelectedStarPlanetIndex >= 0;

            if (!canAddStar) {
                ImGui::Text("星を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("星を追加")) {
                BeginPlacement(
                    "星",
                    mSelectedStarPlanetIndex,
                    [this](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddStar(planetIndex, &placement);
                    });
            }

            if (!canAddStar) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }
}

void StageAddActorPanel::DrawJewelItemCreation()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("ジュエルアイテム追加")) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、ジュエルアイテムを追加できません");
        ImGui::TreePop();
        return;
    }

    DrawPlanetCombo(
        "追加先の惑星##jewelItem",
        mSelectedJewelItemPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newJewelItemModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedJewelItemModel = droppedModelPath;
    }
    ImGui::TextWrapped("モデル: %s", mSelectedJewelItemModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newJewelItemTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedJewelItemTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedJewelItemTexture.c_str());
    ImGui::DragFloat3(
        "初期スケール##jewelItem",
        &mJewelItemScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedJewelItemPlanetIndex >= 0 &&
        !mSelectedJewelItemModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ジュエルアイテムを追加")) {
        const std::string modelPath = mSelectedJewelItemModel;
        const std::string texturePath = mSelectedJewelItemTexture;
        const glm::vec3 scale = mJewelItemScale;
        BeginPlacement(
            "ジュエルアイテム",
            mSelectedJewelItemPlanetIndex,
            [this, modelPath, texturePath, scale](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddJewelItem(
                    planetIndex,
                    modelPath,
                    texturePath,
                    scale,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }
    ImGui::TextDisabled(
        "追加解除まで、ゲーム画面をクリックするたびに配置できます。");
    ImGui::TreePop();
}

void StageAddActorPanel::DrawHazardActorCreation()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("危険アクター追加")) {
        return;
    }

    const auto& planets =
        mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、危険アクターを追加できません");
        ImGui::TreePop();
        return;
    }

    DrawPlanetCombo(
        "追加先の惑星##hazardActor",
        mSelectedHazardActorPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newHazardActorModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedHazardActorModel = droppedModelPath;
    }
    ImGui::TextWrapped(
        "モデル: %s",
        mSelectedHazardActorModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newHazardActorTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedHazardActorTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedHazardActorTexture.empty()
            ? "モデル標準"
            : mSelectedHazardActorTexture.c_str());

    ImGui::DragFloat3(
        "初期スケール##hazardActor",
        &mHazardActorScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");
    ImGui::DragFloat(
        "基準判定半径（スケール1）##hazardActor",
        &mHazardActorTriggerRadius,
        0.01f,
        0.01f,
        100.0f,
        "%.2f");
    ImGui::TextDisabled(
        "判定はアクターの各軸スケールと回転に追従します。");
    ImGui::DragFloat(
        "ダメージ##hazardActor",
        &mHazardActorDamage,
        0.5f,
        0.0f,
        1000.0f,
        "%.1f");
    ImGui::DragFloat(
        "再ダメージ間隔（秒）##hazardActor",
        &mHazardActorDamageIntervalSeconds,
        0.05f,
        0.0f,
        30.0f,
        "%.2f");
    ImGui::TextDisabled(
        "接触時と攻撃を当てた時に同じダメージ・ノックバックを与えます");

    const bool canAdd =
        mSelectedHazardActorPlanetIndex >= 0 &&
        !mSelectedHazardActorModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("危険アクターを追加")) {
        const std::string modelPath =
            mSelectedHazardActorModel;
        const std::string texturePath =
            mSelectedHazardActorTexture;
        const glm::vec3 scale = mHazardActorScale;
        const float triggerRadius =
            mHazardActorTriggerRadius;
        const float damage = mHazardActorDamage;
        const float damageIntervalSeconds =
            mHazardActorDamageIntervalSeconds;
        BeginPlacement(
            "危険アクター",
            mSelectedHazardActorPlanetIndex,
            [this,
             modelPath,
             texturePath,
             scale,
             triggerRadius,
             damage,
             damageIntervalSeconds](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddHazardActor(
                    planetIndex,
                    modelPath,
                    texturePath,
                    scale,
                    triggerRadius,
                    damage,
                    damageIntervalSeconds,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }
    ImGui::TextDisabled(
        "追加解除まで、ゲーム画面をクリックするたびに配置できます");
    ImGui::TreePop();
}

void StageAddActorPanel::DrawBoatArrivalPointCreation()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("ロケット到着ポイント追加")) {
        return;
    }

    const auto& planets =
        mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、ロケット到着ポイントを追加できません");
        ImGui::TreePop();
        return;
    }

    DrawPlanetCombo(
        "追加先の惑星##boatArrivalPoint",
        mSelectedBoatArrivalPointPlanetIndex);

    ImGui::InputTextWithHint(
        "##boatArrivalPointModelSearch",
        "モデル名を検索",
        mBoatArrivalPointModelSearch.data(),
        mBoatArrivalPointModelSearch.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string searchText =
        ToLower(mBoatArrivalPointModelSearch.data());

    ImGui::BeginChild(
        "BoatArrivalPointModelAssetPicker",
        ImVec2(0.0f, 180.0f),
        true);
    for (const std::string& modelPath : modelAssets) {
        if (!searchText.empty() &&
            ToLower(modelPath).find(searchText) ==
                std::string::npos) {
            continue;
        }

        const bool isSelected =
            modelPath == mSelectedBoatArrivalPointModel;
        if (ImGui::Selectable(
                modelPath.c_str(),
                isSelected)) {
            mSelectedBoatArrivalPointModel = modelPath;
        }
    }
    ImGui::EndChild();

    ImGui::Button(
        "モデルアセットをここへドロップ##newBoatArrivalPointModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedBoatArrivalPointModel = droppedModelPath;
    }

    ImGui::TextWrapped(
        "選択中: %s",
        mSelectedBoatArrivalPointModel.empty()
            ? "未選択"
            : mSelectedBoatArrivalPointModel.c_str());
    ImGui::DragFloat3(
        "初期スケール##boatArrivalPoint",
        &mBoatArrivalPointScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedBoatArrivalPointPlanetIndex >= 0 &&
        !mSelectedBoatArrivalPointModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ロケット到着ポイントを追加")) {
        const std::string modelPath =
            mSelectedBoatArrivalPointModel;
        const glm::vec3 scale =
            mBoatArrivalPointScale;
        BeginPlacement(
            "ロケット到着ポイント",
            mSelectedBoatArrivalPointPlanetIndex,
            [this, modelPath, scale](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddBoatArrivalPoint(
                    planetIndex,
                    modelPath,
                    scale,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }

    ImGui::TextDisabled(
        "追加後は一覧・クリック選択・ギズモ・複製・削除を利用できます。");
    ImGui::TreePop();
}

void StageAddActorPanel::DrawPlanetCombo(const char* label, int& selectedPlanetIndex)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        selectedPlanetIndex = -1;
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (selectedPlanetIndex >= static_cast<int>(planets.size())) {
        selectedPlanetIndex = -1;
    }

    std::string previewText = "未選択";
    if (selectedPlanetIndex >= 0) {
        previewText = "惑星 " + std::to_string(selectedPlanetIndex);
    }

    if (ImGui::BeginCombo(label, previewText.c_str())) {
        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            Planet* planet = planets[i];
            if (!planet) {
                continue;
            }

            std::string itemLabel = "惑星 " + std::to_string(i);
            bool isSelected = selectedPlanetIndex == i;

            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                selectedPlanetIndex = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}
