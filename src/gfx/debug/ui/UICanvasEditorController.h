#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ui/UICanvasEditHistory.h"
#include "system/UILoadSystem.h"

#include "imgui.h"
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    void Update(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    void SetSingleSelection(std::size_t index);
    void ToggleSelection(std::size_t index);
    void SelectFromList(std::size_t index, bool additive);
    void SelectExistingTextureFromList(
        const std::string& key,
        bool additive);
    void SelectExistingTextFromList(
        const std::string& key,
        bool additive);
    void ClearSelection();

    bool IsSelected(std::size_t index) const;
    bool IsExistingTextureSelected(const std::string& key) const;
    bool IsExistingTextSelected(const std::string& key) const;
    int GetPrimarySelectedIndex() const;
    SelectionSource GetPrimarySelectionSource() const;
    const std::string& GetPrimaryExistingKey() const;
    std::size_t GetSelectedCount() const;

    void SetOperation(Operation operation);
    Operation GetOperation() const;

    bool DuplicateSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage);
    bool DeleteSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage);
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
    bool ResolveElementTransform(
        const UILoadSystem* uiLoadSystem,
        const ElementReference& element,
        ElementTransform& outTransform) const;
    bool GetSelectionBounds(
        const UILoadSystem* uiLoadSystem,
        glm::vec2& outMin,
        glm::vec2& outMax) const;

    std::vector<ElementReference> PickElementsAt(
        const UILoadSystem* uiLoadSystem,
        const glm::vec2& framebufferPoint) const;
    bool IsPointInsideElement(
        const ElementTransform& transform,
        const glm::vec2& framebufferPoint) const;

    glm::vec2 ImGuiToFramebuffer(const ImVec2& point) const;
    ImVec2 FramebufferToImGui(const glm::vec2& point) const;
    glm::mat4 CreateElementMatrix(const ElementTransform& transform) const;
    glm::mat4 CreateSelectionMatrix(const UILoadSystem* uiLoadSystem) const;
    void ApplySelectionMatrix(
        UILoadSystem* uiLoadSystem,
        const glm::mat4& selectionMatrix);

    void PushUndo(const UILoadSystem* uiLoadSystem);
    void SelectElementsInBox(
        const UILoadSystem* uiLoadSystem,
        const ImVec2& rectMin,
        const ImVec2& rectMax,
        bool additive);

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
