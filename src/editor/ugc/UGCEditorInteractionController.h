#pragma once

#include "editor/core/DebugEditorContext.h"

#include "editor/stage/StageEditorTypes.h"
#include "editor/stage/history/StageEditOperation.h"
#include <glm/glm.hpp>
#include <vector>

class StageActorYamlWriter;
class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditorViewController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragState;
class UGCSwitchConnectionState;

// 配置・選択・ドラッグ・消去の入力を振り分け、編集状態とステージへの反映を連携する。
// 参照で受け取る各モジュールは所有しない。
class UGCEditorInteractionController {
public:
    UGCEditorInteractionController(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
                                   StageActorYamlWriter& stageActorYamlWriter,
                                   StageSelectionController& selectionController,
                                   StageEditCommandController& editCommandController, UGCEditorTutorial& editorTutorial,
                                   UGCEditorToolState& toolState, UGCSelectionDragState& dragState,
                                   UGCSwitchConnectionState& connectionState, UGCEditorViewController& viewController);

    void HandleUndo();
    void HandleRedo();

    // 配置を中断して消しゴムへ切り替える。解除時は可能なら直前の配置プリセットを復元する。
    void ToggleEraser();

    // 配置と消しゴムを解除して選択モードへ切り替える。
    void ActivateSelectionMode();
    void AdjustZoom(float distanceMultiplier);

    // 編集レイヤーを変更し、チュートリアルへ通知する。ドラッグ中は選択対象の高さも変更する。
    void ChangeLayer(int layerDelta);

    // gridX・gridZをグリッド単位の相対移動量として、選択対象を移動・保存する。Gameの初期化が必要。
    void MoveSelectionOnGrid(int gridX, int gridZ);

    // 配置・選択・消去の入力を処理し、Actorの選択表示を同期する。ImGuiフレーム内で呼ぶ。
    void UpdateSceneInteraction();

    // レイヤーを0〜20へ制限して関連モジュールへ反映する。ドラッグ中の対象も移動する。Gameの初期化が必要。
    void ChangeEditLayer(int layerDelta);
    void ToggleVerticalView();
    void SetFixedView(const glm::vec3& viewDirection);
    const glm::vec3& GetViewDirection() const;

private:
    bool SaveSelectionMove(const std::vector<StageActorRef>& actorRefs, const glm::vec3& delta,
                           StageEditGrouping grouping);
    bool TryIntersectEditLayerPlane(const glm::vec3& rayFrom, const glm::vec3& rayTo, float planeHeight,
                                    glm::vec3& outIntersection) const;
    void UpdateSelectionDrag();
    void BeginSelectionDrag();
    void FinishSelectionDrag();
    void SyncEditLayerToPickedActor();
    void HandleEraserInteraction();

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageActorYamlWriter& mStageActorYamlWriter;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSelectionDragState& mDragState;
    UGCSwitchConnectionState& mConnectionState;
    UGCEditorViewController& mViewController;
};
