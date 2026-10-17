#pragma once

#include "gfx/UIRenderer.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ui/UICanvasEditHistory.h"
#include "system/UILoadSystem.h"

#include "imgui.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

// UIキャンバスの選択・ギズモ操作・複製／削除・Undoと保存を管理する。
// contextと操作対象のUILoadSystemは所有しない。UpdateはゲームUI描画後のImGuiフレーム内で呼ぶ。
class UICanvasEditorController {
public:
    enum class Operation {
        Translate,
        Rotate,
        Scale,
    };

    enum class SelectionSource {
        None,
        Custom,
        ExistingTexture,
        ExistingText,
    };

    explicit UICanvasEditorController(DebugEditorContext& context);

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
    struct ElementTransform {
        glm::vec2 center = glm::vec2(0.0f);
        glm::vec2 size = glm::vec2(1.0f);
        float rotationDegrees = 0.0f;
    };

    struct ElementReference {
        SelectionSource source = SelectionSource::None;
        std::size_t customIndex = 0;
        std::string existingKey;

        bool operator==(const ElementReference& other) const = default;
    };

    void ValidateSelection(const UILoadSystem* uiLoadSystem);
    void HandleShortcuts(UILoadSystem* uiLoadSystem, std::string& statusMessage);
    void UpdateCanvasSelection(const UILoadSystem* uiLoadSystem);
    void DrawSelectionOverlay(const UILoadSystem* uiLoadSystem) const;
    void DrawGizmo(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    void SetSingleSelection(const ElementReference& element);
    void ToggleSelection(const ElementReference& element);
    bool IsSelected(const ElementReference& element) const;
    bool ResolveElementTransform(const UILoadSystem* uiLoadSystem, const ElementReference& element,
                                 ElementTransform& outTransform) const;
    bool GetSelectionBounds(const UILoadSystem* uiLoadSystem, glm::vec2& outMin, glm::vec2& outMax) const;

    static std::optional<ElementReference>
    ResolveElementReference(const UILoadSystem& uiLoadSystem, const UIRenderer::RenderedUIElement& renderedElement);
    std::vector<ElementReference> PickElementsAt(const UILoadSystem* uiLoadSystem,
                                                 const glm::vec2& framebufferPoint) const;
    bool IsPointInsideElement(const ElementTransform& transform, const glm::vec2& framebufferPoint) const;

    glm::vec2 ImGuiToFramebuffer(const ImVec2& point) const;
    ImVec2 FramebufferToImGui(const glm::vec2& point) const;
    glm::mat4 CreateElementMatrix(const ElementTransform& transform) const;
    glm::mat4 CreateSelectionMatrix(const UILoadSystem* uiLoadSystem) const;
    bool ResolveTransformStartElement(const ElementReference& element, ElementTransform& outTransform) const;
    void ApplySelectionMatrix(UILoadSystem* uiLoadSystem, const glm::mat4& selectionMatrix);

    void PushUndo(const UILoadSystem* uiLoadSystem);
    void SelectElementsInBox(const UILoadSystem* uiLoadSystem, const ImVec2& rectMin, const ImVec2& rectMax,
                             bool isAdditive);

private:
    DebugEditorContext& mContext;

    std::vector<ElementReference> mSelectedElements;
    std::optional<ElementReference> mPrimarySelection;
    Operation mOperation = Operation::Translate;

    UICanvasEditHistory mEditHistory;

    bool mIsBoxMouseDown = false;
    bool mIsBoxSelecting = false;
    ImVec2 mBoxStart = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxEnd = ImVec2(0.0f, 0.0f);

    bool mHasLastPick = false;
    ImVec2 mLastPickPosition = ImVec2(0.0f, 0.0f);

    bool mIsUsingGizmo = false;
    glm::mat4 mEditingMatrix = glm::mat4(1.0f);
    glm::mat4 mTransformStartSelectionMatrix = glm::mat4(1.0f);
    UICanvasEditSnapshot mTransformStartState;
};
