#pragma once

#include "editor/stage/UGCPresetVisuals.h"

#include <optional>
#include <string>

// 配置・選択・消しゴムのモード遷移と、各UIで共有する編集設定を保持する。
class UGCEditorToolState {
public:
    void ActivatePreset(UGCPresetKind presetKind);
    void ActivateSelection();

    // 解除後に復元するプリセットを記録する。既に消しゴムモードなら変更しない。
    void EnterEraser(std::optional<UGCPresetKind> presetToRestore);

    // 配置側での復元結果に合わせ、プリセットを復元または解除する。
    void LeaveEraser(bool didRestorePreviousPreset);
    void DeactivateEraser();

    // 選択モードへ戻し、通知・編集レイヤー・足場サイズを初期化する。
    void ResetForLoadedWork();

    bool isEraserMode = false;
    std::optional<UGCPresetKind> activePresetKind;
    std::optional<UGCPresetKind> presetBeforeEraser;
    std::string statusMessage;
    int editLayer = 0;
    int platformFootprintSideLength = 1;
};
