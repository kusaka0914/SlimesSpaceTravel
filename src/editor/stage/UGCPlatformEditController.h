#pragma once

#include "editor/stage/StageEditOperation.h"

#include "editor/stage/StageEditorTypes.h"
#include "editor/stage/UGCPlatformDocument.h"

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
    void SetEditExecutor(StageEditExecutor executeEdit);
    void SetGridLayer(int gridLayer) { mGridLayer = gridLayer; }

    // マウス位置のセルを消去し、同じドラッグ中のUndo記録をまとめる。変更できない場合は false。
    bool TryEraseCell();

    // 消去操作の状態をリセットする。マウスを離したときやツール切り替え時に呼ぶ。
    void EndEraseGesture();
    bool TryTranslateMovingPlatformDestinations(const std::vector<StageActorRef>& actorRefs,
                                                const glm::vec3& worldDelta);
    bool TrySaveMovingPlatformDestinationTranslation(const std::vector<StageActorRef>& actorRefs,
                                                     const glm::vec3& worldDelta);

private:
    bool ExecuteErase(const StageEditOperation& remove);

    DebugEditorContext& mContext;
    UGCPlatformCellService& mPlatformCellService;
    StageActorPlacementResolver& mPlacementResolver;
    StageSelectionController* mSelectionController = nullptr;
    StageEditExecutor mExecuteEdit;
    int mGridLayer = 0;
    bool mHasSavedEraseStroke = false;
    std::optional<glm::ivec3> mLastErasedCell;
    std::optional<UGCGeneratedPlatformRegion> mMovingDestinationEraseRegion;
    std::optional<int> mMovingDestinationEraseLayer;
};
