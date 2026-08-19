#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "actor/enemy/EnemyPresetRepository.h"

#include <array>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

class StageSelectionController;
class Actor;

enum class UGCPresetKind {
    NormalPlatform,
    NormalEnemy,
    EllipsePlanet,
    PressureSwitch,
    GoalStar,
};

class StageAddActorPanel : public DebugPanel {
public:
    explicit StageAddActorPanel(DebugEditorContext& context);

    void Draw() override;
    void UpdatePlacement();
    void SetSelectionController(StageSelectionController* selectionController);
    void SetPushUndoCallback(std::function<void()> pushUndoCallback);
    void SetUGCEditLayer(int gridLayer) { mUGCEditLayer = gridLayer; }
    void SetUGCPlatformFootprintSideLength(int sideLength)
    {
        mUGCPlatformFootprintSideLength = sideLength;
    }
    bool BeginDuplicatePlacement(const StageActorRef& sourceRef);
    bool ActivateUGCPreset(UGCPresetKind presetKind);
    bool TryEraseUGCPlatformCell();
    bool TryTranslateUGCPlatformCells(
        const StageActorRef& actorRef,
        const glm::vec3& worldDelta);
    bool TryTranslateUGCPlatformCells(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool IsPlacementActive() const { return static_cast<bool>(mPlacementCreator); }
    const std::optional<glm::vec3>& GetPlacementPreviewPosition() const
    {
        return mPlacementPreviewPosition;
    }
    void CancelPlacement();

private:
    void DrawPlanetCombo(const char* label, int& selectedPlanetIndex);
    void DrawBoatArrivalPointCreation();
    void DrawJewelItemCreation();
    void DrawHazardActorCreation();
    void BeginPlacement(const std::string& displayName, int fallbackPlanetIndex,
                        std::function<bool(int, const StageActorPlacement&)> placementCreator,
                        bool snapToGridIntersections = true,
                        bool continuousPlacement = false,
                        bool autoStackUGCPlatforms = false,
                        bool showUGCPlatformPreview = false);
    int ResolveHitPlanetIndex(Actor* hitActor, int fallbackPlanetIndex) const;
    bool TryCreateUGCFallbackPlacement(
        const glm::vec3& rayFrom,
        const glm::vec3& rayTo,
        StageActorPlacement& outPlacement) const;

private:
    StageActorCreateService mCreateService;
    StageSelectionController* mSelectionController = nullptr;
    std::function<void()> mPushUndoCallback;
    std::function<bool(int, const StageActorPlacement&)> mPlacementCreator;
    std::string mPlacementDisplayName;
    std::string mPlacementStatus;
    int mPlacementFallbackPlanetIndex = -1;
    bool mSnapPlacementToGridIntersections = true;
    bool mIsContinuousPlacement = false;
    bool mAutoStackUGCPlatforms = false;
    bool mShowUGCPlatformPreview = false;
    std::string mUGCPlacementPreviewModelPath;
    glm::vec3 mUGCPlacementPreviewModelScale{1.0f};
    bool mIsContinuousPlacementStrokeActive = false;
    std::optional<int> mUGCContinuousPlacementLayer;
    std::optional<glm::ivec3> mLastPaintedUGCCell;
    std::optional<glm::vec3> mPlacementPreviewPosition;
    int mUGCEditLayer = 0;
    int mUGCPlatformFootprintSideLength = 1;
    std::optional<glm::ivec3> mLastErasedUGCCell;

    int mSelectedPlanetModelIndex = 0;
    std::string mSelectedPlanetModelPath = "planet.obj";

    int mSelectedEnemyTypeIndex = 0;
    int mSelectedEnemyPlanetIndex = -1;
    std::vector<EnemyPresetDefinition> mEnemyPresets;
    std::uint64_t mLoadedEnemyPresetRevision = 0;
    bool mEnemyPresetsLoaded = false;
    std::string mEnemyPresetLoadError;

    int mSelectedPlatformPlanetIndex = -1;
    int mSelectedPlatformModelIndex = 0;
    std::string mSelectedPlatformModelPath = "platform.obj";
    glm::vec3 mPlatformScale = glm::vec3(1.0f, 1.0f, 1.0f);
    std::string mRideMovingPlatformStatus;

    int mSelectedCrystalPlanetIndex = -1;
    int mSelectedCrystalTypeIndex = 0;

    int mSelectedNPCPlanetIndex = -1;
    std::string mSelectedNPCModel;
    std::array<char, 128> mNPCModelSearch = {};
    std::array<char, 128> mNPCName = {};
    std::vector<std::array<char, 1024>> mNPCTalkTexts;
    float mNPCTalkRadius = 0.75f;
    float mNPCScale = 1.0f;
    std::string mNPCStatus;

    int mSelectedTutorialTriggerPlanetIndex = -1;
    std::string mSelectedTutorialTriggerModel =
        "selectField.obj";
    std::array<char, 128>
        mTutorialTriggerModelSearch = {};
    std::vector<std::array<char, 1024>>
        mTutorialTriggerTalkTexts;
    glm::vec3 mTutorialTriggerScale =
        glm::vec3(2.0f);
    std::string mTutorialTriggerStatus;

    int mSelectedBoatPartsPlanetIndex = -1;
    int mSelectedBoatPartsTypeIndex = 0;

    int mSelectedBoatStartPlanetIndex = -1;
    int mSelectedBoatDestPlanetIndex = -1;
    int mSelectedBoatDestStage = 0;

    int mSelectedBoatArrivalPointPlanetIndex = -1;
    std::string mSelectedBoatArrivalPointModel = "platform.obj";
    std::array<char, 128> mBoatArrivalPointModelSearch = {};
    glm::vec3 mBoatArrivalPointScale = glm::vec3(0.4f);

    int mSelectedStarPlanetIndex = -1;

    int mSelectedJewelItemPlanetIndex = -1;
    std::string mSelectedJewelItemModel = "crystal.obj";
    std::string mSelectedJewelItemTexture = "textures/jewel.png";
    glm::vec3 mJewelItemScale = glm::vec3(0.22f);

    int mSelectedHazardActorPlanetIndex = -1;
    std::string mSelectedHazardActorModel = "crystal.obj";
    std::string mSelectedHazardActorTexture;
    glm::vec3 mHazardActorScale = glm::vec3(0.75f);
    float mHazardActorTriggerRadius = 1.0f;
    float mHazardActorDamage = 1.0f;
    float mHazardActorDamageIntervalSeconds = 1.0f;

    int mSelectedStageObjectPlanetIndex = -1;
    std::string mSelectedStageObjectModel;
    std::array<char, 128> mStageObjectSearch = {};
    bool mStageObjectCollisionEnabled = true;
    std::string mStageObjectStatus;
};
