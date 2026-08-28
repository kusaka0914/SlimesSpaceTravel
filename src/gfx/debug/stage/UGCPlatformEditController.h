#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

class StageActorPlacementResolver;
class StageSelectionController;
class UGCPlatformCellService;
struct DebugEditorContext;

class UGCPlatformEditController {
public:
    UGCPlatformEditController(
        DebugEditorContext& context,
        UGCPlatformCellService& platformCellService,
        StageActorPlacementResolver& placementResolver);

    void SetSelectionController(
        StageSelectionController* selectionController);
    void SetPushUndoCallback(std::function<void()> pushUndoCallback);
    void SetGridLayer(int gridLayer) { mGridLayer = gridLayer; }

    bool TryEraseCell();
    bool TryTranslateCells(
        const StageActorRef& actorRef,
        const glm::vec3& worldDelta);
    bool TryTranslateCells(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool TryTranslateMovingPlatformDestinations(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool TrySaveMovingPlatformDestinationTranslation(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);

private:
    DebugEditorContext& mContext;
    UGCPlatformCellService& mPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
    StageSelectionController* mSelectionController = nullptr;
    std::function<void()> mPushUndoCallback;
    int mGridLayer = 0;
    std::optional<glm::ivec3> mLastErasedCell;
};
