#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "system/UILoadSystem.h"

#include "imgui.h"
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

class UICanvasEditorController {
public:
    enum class Operation {
        Translate,
        Rotate,
        Scale,
    };

    explicit UICanvasEditorController(DebugEditorContext& context);

    void Update(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    void SetSingleSelection(std::size_t index);
    void ToggleSelection(std::size_t index);
    void SelectFromList(std::size_t index, bool additive);
    void ClearSelection();

    bool IsSelected(std::size_t index) const;
    int GetPrimarySelectedIndex() const;
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

    void ValidateSelection(const UILoadSystem* uiLoadSystem);
    void HandleShortcuts(UILoadSystem* uiLoadSystem, std::string& statusMessage);
    void UpdateCanvasSelection(const UILoadSystem* uiLoadSystem);
    void DrawSelectionOverlay(const UILoadSystem* uiLoadSystem) const;
    void DrawGizmo(UILoadSystem* uiLoadSystem, std::string& statusMessage);

    bool GetElementTransform(
        const UILoadSystem::CustomElement& element,
        ElementTransform& outTransform) const;
    bool GetSelectionBounds(
        const UILoadSystem* uiLoadSystem,
        glm::vec2& outMin,
        glm::vec2& outMax) const;

    std::vector<std::size_t> PickElementsAt(
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

    std::unordered_set<std::size_t> mSelectedIndices;
    int mPrimarySelectedIndex = -1;
    Operation mOperation = Operation::Translate;

    std::vector<std::vector<UILoadSystem::CustomElement>> mUndoStack;

    bool mIsBoxMouseDown = false;
    bool mIsBoxSelecting = false;
    ImVec2 mBoxStart = ImVec2(0.0f, 0.0f);
    ImVec2 mBoxEnd = ImVec2(0.0f, 0.0f);

    bool mHasLastPick = false;
    ImVec2 mLastPickPosition = ImVec2(0.0f, 0.0f);

    bool mIsUsingGizmo = false;
    glm::mat4 mEditingMatrix = glm::mat4(1.0f);
    glm::mat4 mTransformStartSelectionMatrix = glm::mat4(1.0f);
    std::vector<UILoadSystem::CustomElement> mTransformStartElements;
};
