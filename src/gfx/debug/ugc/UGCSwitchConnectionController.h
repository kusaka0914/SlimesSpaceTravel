#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"

#include <optional>

class Actor;
class StageAddActorPanel;
class StageSelectionController;
class UGCEditorToolState;

// スイッチの接続先選択とYAMLへの接続変更を連携する。各編集モジュールは所有しない。
class UGCSwitchConnectionController {
public:
    UGCSwitchConnectionController(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
                                  StageSelectionController& selectionController, UGCEditorToolState& toolState,
                                  UGCSwitchConnectionState& connectionState);

    // 選択先を処理したらtrue。対象外の選択では待機を続け、有効な対象では接続処理後に待機を解除する。
    // 接続変更時にはステージを再読み込みするため、呼び出し後のActor参照の再利用に注意する。
    bool CompletePendingConnection(Actor* selectedActor, const std::optional<StageActorRef>& selectedRef);

    // 配置・消しゴム・現在の選択を解除して、指定スイッチの接続先選択を開始する。
    void BeginConnection(const StageActorRef& switchRef, UGCSwitchConnectionAction action);

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    UGCEditorToolState& mToolState;
    UGCSwitchConnectionState& mConnectionState;
};
