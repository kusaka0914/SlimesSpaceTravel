#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ui/UICanvasEditHistory.h"
#include "gfx/debug/ui/UICanvasElementGeometry.h"
#include "gfx/debug/ui/UICanvasSelectionController.h"
#include "gfx/debug/ui/UICanvasTransformController.h"

#include <string>

// UIキャンバス編集の窓口。選択・ギズモの更新順序と、複製／削除・Undo・保存の連携を管理する。
// contextと操作対象のUILoadSystemは所有しない。UpdateはゲームUI描画後のImGuiフレーム内で呼ぶ。
class UICanvasEditorController {
public:
    using Operation = UICanvasOperation;
    using SelectionSource = UICanvasSelectionSource;

    explicit UICanvasEditorController(DebugEditorContext& context);

    // 内部モジュールが本クラスのメンバを参照するため、コピー・移動による参照先のずれを防ぐ。
    UICanvasEditorController(const UICanvasEditorController&) = delete;
    UICanvasEditorController& operator=(const UICanvasEditorController&) = delete;

    // 選択の検証・入力・選択枠・ギズモを更新する。uiLoadSystemまたはuiRendererが未設定なら何もしない。
    void Update(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    // indexはCustomElements内の添字。範囲は次回Updateで検証する。
    void SetSingleSelection(std::size_t index);
    void ToggleSelection(std::size_t index);

    // isAdditiveがtrueなら選択を反転し、falseなら単独選択にする。既存UIのkeyはscreen.id形式。
    void SelectFromList(std::size_t index, bool isAdditive);
    void SelectExistingTextureFromList(const std::string& key, bool isAdditive);
    void SelectExistingTextFromList(const std::string& key, bool isAdditive);

    void ClearSelection();

    bool IsSelected(std::size_t index) const;
    bool IsExistingTextureSelected(const std::string& key) const;
    bool IsExistingTextSelected(const std::string& key) const;

    // 主選択がカスタムUIなら添字、それ以外は-1を返す。
    int GetPrimarySelectedIndex() const;
    SelectionSource GetPrimarySelectionSource() const;

    // 主選択の既存UIキー。主選択なし・カスタムUIの場合は空文字列。
    const std::string& GetPrimaryExistingKey() const;
    std::size_t GetSelectedCount() const;

    // ギズモ操作中のモード変更は無視する。
    void SetOperation(Operation operation);
    Operation GetOperation() const;

    // 複製・削除はカスタムUIだけが対象。Undoを記録し、変更と保存を試みる。
    // 戻り値はメモリ上の変更の成否。保存失敗でも変更は残り、statusMessageに結果を設定する。
    bool DuplicateSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage);
    bool DeleteSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    // 履歴を復元して選択を解除し、両UI定義の保存を試みる。保存失敗でも復元できればtrue。
    bool RestoreUndo(UILoadSystem* uiLoadSystem, std::string& statusMessage);

private:
    void HandleShortcuts(UILoadSystem* uiLoadSystem, std::string& statusMessage);
    void PushUndo(const UILoadSystem* uiLoadSystem);

    DebugEditorContext& mContext;
    UICanvasEditHistory mEditHistory;
    UICanvasElementGeometry mGeometry;
    UICanvasSelectionController mSelection;
    UICanvasTransformController mTransformController;
};
