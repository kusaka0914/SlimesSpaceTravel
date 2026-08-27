#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

class StageActorCreateService;
class StageActorPlacementResolver;
class StageSelectionController;
class UGCPlatformCellService;
class Platform;
struct DebugEditorContext;

class StageActorPlacementController {
public:
    StageActorPlacementController(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        UGCPlatformCellService& platformCellService,
        StageActorPlacementResolver& placementResolver);

    void SetSelectionController(
        StageSelectionController* selectionController);
    void SetPushUndoCallback(
        std::function<void()> pushUndoCallback);
    void SetUGCEditLayer(int gridLayer) { mUGCEditLayer = gridLayer; }
    void SetUGCPlatformFootprintSideLength(int sideLength)
    {
        mUGCPlatformFootprintSideLength = sideLength;
    }

    bool BeginDuplicatePlacement(const StageActorRef& sourceRef);
    void BeginPlacement(
        const std::string& displayName,
        int fallbackPlanetIndex,
        std::function<bool(int, const StageActorPlacement&)> placementCreator,
        bool snapToGridIntersections = true,
        bool continuousPlacement = false,
        bool autoStackUGCPlatforms = false,
        bool showUGCPlatformPreview = false);
    void BeginTargetPlatformSelection(
        const std::string& displayName,
        const glm::vec3& sourcePreviewPosition,
        std::function<bool(Platform*)> targetSelector,
        std::function<bool()> placementWithoutTargetCreator,
        std::function<void()> continuePlacementCallback);
    void BeginMovingPlatformStrokePlacement(
        std::function<bool(
            int,
            const std::vector<glm::ivec3>&,
            const glm::ivec3&)> movingPlatformCreator);
    void UpdatePlacement();
    void CancelPlacement();
    void SetPlacementDisplayName(const std::string& displayName)
    {
        mPlacementDisplayName = displayName;
    }
    void SetPlacementPrompt(
        const std::string& displayName,
        const std::string& status)
    {
        mPlacementDisplayName = displayName;
        mPlacementStatus = status;
    }
    void SetPlacementPreviewModel(
        const std::string& modelPath,
        const glm::vec3& modelScale,
        const std::string& textureOverridePath = "")
    {
        mUGCPlacementPreviewModelPath = modelPath;
        mUGCPlacementPreviewModelScale = modelScale;
        mUGCPlacementPreviewTextureOverridePath = textureOverridePath;
    }
    void SetFixedPlacementPreviewPositions(
        const std::vector<glm::vec3>& positions)
    {
        mFixedPlacementPreviewPositions = positions;
    }
    void SetMovingPlatformPathStartPosition(
        const std::optional<glm::vec3>& startPosition)
    {
        mMovingPlatformPathStartPosition = startPosition;
    }

    bool IsPlacementActive() const
    {
        return static_cast<bool>(mPlacementCreator) ||
            static_cast<bool>(mTargetPlatformSelector) ||
            static_cast<bool>(mMovingPlatformCreator);
    }
    const std::optional<glm::vec3>& GetPlacementPreviewPosition() const
    {
        return mPlacementPreviewPosition;
    }
    const std::string& GetPlacementDisplayName() const
    {
        return mPlacementDisplayName;
    }
    const std::string& GetPlacementStatus() const
    {
        return mPlacementStatus;
    }

private:
    void UpdateTargetPlatformSelection();
    void UpdateMovingPlatformStrokePlacement();
    void PrepareNextMovingPlatformPlacement();
    std::vector<glm::vec3> CalculateStrokePreviewPositions(
        const glm::ivec3& translationCells) const;

    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    UGCPlatformCellService& mUGCPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
    StageSelectionController* mSelectionController = nullptr;
    std::function<void()> mPushUndoCallback;
    std::function<bool(int, const StageActorPlacement&)> mPlacementCreator;
    std::function<bool(Platform*)> mTargetPlatformSelector;
    std::function<bool()> mPlacementWithoutTargetCreator;
    std::function<void()> mContinuePlacementCallback;
    std::function<bool(
        int,
        const std::vector<glm::ivec3>&,
        const glm::ivec3&)> mMovingPlatformCreator;
    std::string mPlacementDisplayName;
    std::string mPlacementStatus;
    int mPlacementFallbackPlanetIndex = -1;
    bool mSnapPlacementToGridIntersections = true;
    bool mIsContinuousPlacement = false;
    bool mAutoStackUGCPlatforms = false;
    bool mShowUGCPlatformPreview = false;
    std::string mUGCPlacementPreviewModelPath;
    glm::vec3 mUGCPlacementPreviewModelScale{1.0f};
    std::string mUGCPlacementPreviewTextureOverridePath;
    bool mIsContinuousPlacementStrokeActive = false;
    std::optional<int> mUGCContinuousPlacementLayer;
    std::optional<glm::ivec3> mLastPaintedUGCCell;
    std::optional<glm::vec3> mPlacementPreviewPosition;
    std::optional<glm::vec3> mMovingPlatformPathStartPosition;
    std::optional<glm::vec3> mTargetSelectionSourcePreviewPosition;
    std::vector<glm::vec3> mFixedPlacementPreviewPositions;
    std::vector<glm::ivec3> mMovingPlatformStartCells;
    bool mWasMovingPlatformStrokeActive = false;
    bool mIsMovingPlatformDestinationPlacementActive = false;
    bool mShouldWaitForMovingPlatformSourceRelease = false;
    int mUGCEditLayer = 0;
    int mUGCPlatformFootprintSideLength = 1;
};
