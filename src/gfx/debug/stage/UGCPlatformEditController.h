#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/stage/UGCPlatformDocument.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

class StageActorPlacementResolver;
class StageSelectionController;
class UGCPlatformCellService;
struct DebugEditorContext;

// UGCセルの消去・移動操作を仲介する。連続消去では一連の操作につきUndoを1回記録する。
class UGCPlatformEditController {
public:
    UGCPlatformEditController(DebugEditorContext& context, UGCPlatformCellService& platformCellService,
                              StageActorPlacementResolver& placementResolver);

    void SetSelectionController(StageSelectionController* selectionController);
    void SetPushUndoCallback(std::function<void()> pushUndoCallback);
    void SetGridLayer(int gridLayer) { mGridLayer = gridLayer; }

    // マウス位置のセルを消去し、同じドラッグ中のUndo記録をまとめる。変更できない場合は false。
    bool TryEraseCell();

    // 消去操作の状態をリセットする。マウスを離したときやツール切り替え時に呼ぶ。
    void EndEraseGesture();
    bool TryTranslateCells(const StageActorRef& actorRef, const glm::vec3& worldDelta);
    bool TryTranslateCells(const std::vector<StageActorRef>& actorRefs, const glm::vec3& worldDelta);
    bool TryTranslateMovingPlatformDestinations(const std::vector<StageActorRef>& actorRefs,
                                                const glm::vec3& worldDelta);
    bool TrySaveMovingPlatformDestinationTranslation(const std::vector<StageActorRef>& actorRefs,
                                                     const glm::vec3& worldDelta);

private:
    DebugEditorContext& mContext;
    UGCPlatformCellService& mPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
    StageSelectionController* mSelectionController = nullptr;
    std::function<void()> mPushUndoCallback;
    int mGridLayer = 0;
    bool mHasPushedEraseStrokeUndo = false;
    std::optional<glm::ivec3> mLastErasedCell;
    std::optional<UGCGeneratedPlatformRegion> mMovingDestinationEraseRegion;
    std::optional<int> mMovingDestinationEraseLayer;
};
