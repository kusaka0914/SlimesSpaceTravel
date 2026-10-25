#pragma once

#include "gfx/UIRenderer.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/ui/UICanvasEditorTypes.h"
#include "system/UILoadSystem.h"

#include "imgui.h"

#include <optional>
#include <vector>

class UICanvasElementGeometry;

// 単独・複数・範囲選択、重なった要素の巡回選択と選択枠の描画を管理する。
// context・geometryと選択先のUI定義は所有しない。
class UICanvasSelectionController {
public:
    explicit UICanvasSelectionController(DebugEditorContext& context, const UICanvasElementGeometry& geometry);

    void ValidateSelection(const UILoadSystem* uiLoadSystem);

    // UI描画後のImGuiフレーム内で呼ぶ。ギズモ操作中は新しい選択入力を処理しない。
    void UpdateCanvasSelection(const UILoadSystem* uiLoadSystem, bool isUsingGizmo);
    void DrawSelectionOverlay(const UILoadSystem* uiLoadSystem) const;

    void SetSingleSelection(const UICanvasElementReference& element);
    void ToggleSelection(const UICanvasElementReference& element);
    void ClearSelection();
    bool IsSelected(const UICanvasElementReference& element) const;

    // 選択を置き換え、最後の要素を主選択にする。空なら主選択も解除する。
    void SetElements(std::vector<UICanvasElementReference> elements);
    const std::vector<UICanvasElementReference>& GetElements() const;
    const std::optional<UICanvasElementReference>& GetPrimarySelection() const;

private:
    static std::optional<UICanvasElementReference>
    ResolveElementReference(const UILoadSystem& uiLoadSystem, const UIRenderer::RenderedUIElement& renderedElement);
    std::vector<UICanvasElementReference> PickElementsAt(const UILoadSystem* uiLoadSystem,
                                                         const glm::vec2& framebufferPoint) const;
    void SelectElementsInBox(const UILoadSystem* uiLoadSystem, const ImVec2& rectMin, const ImVec2& rectMax,
                             bool isAdditive);

    DebugEditorContext& mContext;
    const UICanvasElementGeometry& mGeometry;
    std::vector<UICanvasElementReference> mSelectedElements;
    std::optional<UICanvasElementReference> mPrimarySelection;
    bool mIsBoxMouseDown = false;
    bool mIsBoxSelecting = false;
    ImVec2 mBoxStart = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxEnd = ImVec2(0.0f, 0.0f);
    bool mHasLastPick = false;
    ImVec2 mLastPickPosition = ImVec2(0.0f, 0.0f);
};
