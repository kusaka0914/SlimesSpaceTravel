#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ui/UICanvasEditHistory.h"
#include "gfx/debug/ui/UICanvasEditorTypes.h"

#include <string>

class UICanvasElementGeometry;
class UICanvasSelectionController;

// 選択対象の移動・回転・拡縮をギズモで編集する。操作開始時の状態を基準に変換し、終了時に保存する。
// context・geometry・selection・editHistoryは所有しない。
class UICanvasTransformController {
public:
    UICanvasTransformController(DebugEditorContext& context, const UICanvasElementGeometry& geometry,
                                const UICanvasSelectionController& selection, UICanvasEditHistory& editHistory);

    // ゲームUI描画後のImGuiフレーム内で呼ぶ。操作開始時にUndoを1回記録し、終了時に両UI定義を保存する。
    // 保存に失敗してもメモリ上の変更は残り、statusMessageへ結果を設定する。
    void DrawGizmo(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    // ギズモ操作中のモード変更は無視する。
    void SetOperation(UICanvasOperation operation);
    UICanvasOperation GetOperation() const;
    bool IsUsingGizmo() const;

private:
    bool ResolveTransformStartElement(const UICanvasElementReference& element,
                                      UICanvasElementTransform& outTransform) const;
    void ApplySelectionMatrix(UILoadSystem* uiLoadSystem, const glm::mat4& selectionMatrix);

    DebugEditorContext& mContext;
    const UICanvasElementGeometry& mGeometry;
    const UICanvasSelectionController& mSelection;
    UICanvasEditHistory& mEditHistory;
    UICanvasOperation mOperation = UICanvasOperation::Translate;
    bool mIsUsingGizmo = false;
    glm::mat4 mEditingMatrix = glm::mat4(1.0f);
    glm::mat4 mTransformStartSelectionMatrix = glm::mat4(1.0f);
    UICanvasEditSnapshot mTransformStartState;
};
