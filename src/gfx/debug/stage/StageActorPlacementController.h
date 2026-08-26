#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>

class StageActorCreateService;
class StageActorPlacementResolver;
class StageSelectionController;
class UGCPlatformCellService;
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
        const glm::vec3& modelScale)
    {
        mUGCPlacementPreviewModelPath = modelPath;
        mUGCPlacementPreviewModelScale = modelScale;
    }

    bool IsPlacementActive() const
    {
        return static_cast<bool>(mPlacementCreator);
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
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    UGCPlatformCellService& mUGCPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
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
};
