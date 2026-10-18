#pragma once

#include "gfx/UIRenderer.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/ui/UICanvasEditorTypes.h"

#include "imgui.h"

#include <vector>

// 要素の画面上の形状、選択範囲、ImGuiとフレームバッファの座標変換を計算する。contextは所有しない。
class UICanvasElementGeometry {
public:
    explicit UICanvasElementGeometry(DebugEditorContext& context);

    static constexpr float minimumElementSizePixels = 4.0f;

    // 対象の定義または描画情報が取得できない場合はfalse。
    bool ResolveElementTransform(const UILoadSystem* uiLoadSystem, const UICanvasElementReference& element,
                                 UICanvasElementTransform& outTransform) const;

    // 回転後の各要素を含む軸平行の範囲を返す。有効な要素がなければfalse。
    bool GetSelectionBounds(const UILoadSystem* uiLoadSystem, const std::vector<UICanvasElementReference>& elements,
                            glm::vec2& outMin, glm::vec2& outMax) const;

    // 単独選択では要素の回転を使い、複数選択では全体の軸平行範囲から行列を作る。対象なしなら単位行列。
    glm::mat4 CreateSelectionMatrix(const UILoadSystem* uiLoadSystem,
                                    const std::vector<UICanvasElementReference>& elements) const;

    glm::vec2 ImGuiToFramebuffer(const ImVec2& point) const;
    ImVec2 FramebufferToImGui(const glm::vec2& point) const;

    static glm::vec2 RotatePoint(const glm::vec2& point, float degrees);
    static glm::mat4 CreateElementMatrix(const UICanvasElementTransform& transform);
    static bool IsPointInsideElement(const UICanvasElementTransform& transform, const glm::vec2& framebufferPoint);

private:
    DebugEditorContext& mContext;
};
