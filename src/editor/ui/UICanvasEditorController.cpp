#include "editor/ui/UICanvasEditorController.h"

#include "ImGuizmo.h"
#include "imgui.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace {
using ElementReference = UICanvasElementReference;
} // namespace

UICanvasEditorController::UICanvasEditorController(DebugEditorContext& context)
    : mContext(context),
      mGeometry(context),
      mSelection(context, mGeometry),
      mTransformController(context, mGeometry, mSelection, mEditHistory)
{
}

bool UICanvasEditorController::HandlesViewportClick(const UILoadSystem* uiLoadSystem) const
{
    return mTransformController.IsUsingGizmo() || ImGuizmo::IsOver() || ImGuizmo::IsUsing() ||
           mSelection.HasElementAtMouse(uiLoadSystem);
}

void UICanvasEditorController::Update(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return;
    }

    mSelection.ValidateSelection(uiLoadSystem);
    HandleShortcuts(uiLoadSystem, statusMessage);
    mSelection.UpdateCanvasSelection(uiLoadSystem, mTransformController.IsUsingGizmo());
    mSelection.DrawSelectionOverlay(uiLoadSystem);
    mTransformController.DrawGizmo(uiLoadSystem, statusMessage);
}

void UICanvasEditorController::SetSingleSelection(std::size_t index)
{
    mSelection.SetSingleSelection(ElementReference{SelectionSource::Custom, index, std::string()});
}

void UICanvasEditorController::ToggleSelection(std::size_t index)
{
    mSelection.ToggleSelection(ElementReference{SelectionSource::Custom, index, std::string()});
}

void UICanvasEditorController::SelectFromList(std::size_t index, bool isAdditive)
{
    if (isAdditive) {
        ToggleSelection(index);
    } else {
        SetSingleSelection(index);
    }
}

void UICanvasEditorController::SelectExistingTextureFromList(const std::string& key, bool isAdditive)
{
    const ElementReference element{SelectionSource::ExistingTexture, 0, key};
    if (isAdditive) {
        mSelection.ToggleSelection(element);
    } else {
        mSelection.SetSingleSelection(element);
    }
}

void UICanvasEditorController::SelectExistingTextFromList(const std::string& key, bool isAdditive)
{
    const ElementReference element{SelectionSource::ExistingText, 0, key};
    if (isAdditive) {
        mSelection.ToggleSelection(element);
    } else {
        mSelection.SetSingleSelection(element);
    }
}

void UICanvasEditorController::ClearSelection()
{
    mSelection.ClearSelection();
}

bool UICanvasEditorController::IsSelected(std::size_t index) const
{
    return mSelection.IsSelected(ElementReference{SelectionSource::Custom, index, std::string()});
}

bool UICanvasEditorController::IsExistingTextureSelected(const std::string& key) const
{
    return mSelection.IsSelected(ElementReference{SelectionSource::ExistingTexture, 0, key});
}

bool UICanvasEditorController::IsExistingTextSelected(const std::string& key) const
{
    return mSelection.IsSelected(ElementReference{SelectionSource::ExistingText, 0, key});
}

int UICanvasEditorController::GetPrimarySelectedIndex() const
{
    const auto& primarySelection = mSelection.GetPrimarySelection();
    if (!primarySelection || primarySelection->source != SelectionSource::Custom) {
        return -1;
    }
    return static_cast<int>(primarySelection->customIndex);
}

UICanvasEditorController::SelectionSource UICanvasEditorController::GetPrimarySelectionSource() const
{
    const auto& primarySelection = mSelection.GetPrimarySelection();
    return primarySelection ? primarySelection->source : SelectionSource::None;
}

const std::string& UICanvasEditorController::GetPrimaryExistingKey() const
{
    const auto& primarySelection = mSelection.GetPrimarySelection();
    static const std::string emptyKey;
    return primarySelection ? primarySelection->existingKey : emptyKey;
}

std::size_t UICanvasEditorController::GetSelectedCount() const
{
    return mSelection.GetElements().size();
}

void UICanvasEditorController::SetOperation(Operation operation)
{
    mTransformController.SetOperation(operation);
}

UICanvasEditorController::Operation UICanvasEditorController::GetOperation() const
{
    return mTransformController.GetOperation();
}

bool UICanvasEditorController::DuplicateSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    if (!uiLoadSystem || mSelection.GetElements().empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> sourceIndices;
    for (const ElementReference& element : mSelection.GetElements()) {
        if (element.source == SelectionSource::Custom) {
            sourceIndices.push_back(element.customIndex);
        }
    }
    if (sourceIndices.empty()) {
        mEditHistory.DiscardLatest();
        statusMessage = "コード連携UIは複製できません";
        return false;
    }
    std::sort(sourceIndices.begin(), sourceIndices.end());

    std::vector<ElementReference> duplicatedElements;
    for (std::size_t sourceIndex : sourceIndices) {
        const std::optional<std::size_t> duplicatedIndex = uiLoadSystem->DuplicateCustomElement(sourceIndex);
        if (duplicatedIndex) {
            duplicatedElements.push_back(ElementReference{SelectionSource::Custom, *duplicatedIndex, std::string()});
        }
    }

    if (duplicatedElements.empty()) {
        mEditHistory.DiscardLatest();
        statusMessage = "要素の複製に失敗しました";
        return false;
    }

    mSelection.SetElements(std::move(duplicatedElements));

    const bool wasSaved = uiLoadSystem->SaveCustomUI();
    statusMessage = wasSaved ? "選択中のUIを複製して保存しました" : "UIは複製しましたが保存に失敗しました";
    return true;
}

bool UICanvasEditorController::DeleteSelected(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    if (!uiLoadSystem || mSelection.GetElements().empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> deleteIndices;
    for (const ElementReference& element : mSelection.GetElements()) {
        if (element.source == SelectionSource::Custom) {
            deleteIndices.push_back(element.customIndex);
        }
    }
    if (deleteIndices.empty()) {
        mEditHistory.DiscardLatest();
        statusMessage = "コード連携UIは削除できません";
        return false;
    }

    // 後ろの要素から削除し、まだ処理していない添字をずらさない。
    std::sort(deleteIndices.rbegin(), deleteIndices.rend());

    bool wasDeleted = false;
    for (std::size_t index : deleteIndices) {
        wasDeleted |= uiLoadSystem->RemoveCustomElement(index);
    }

    if (!wasDeleted) {
        mEditHistory.DiscardLatest();
        statusMessage = "UIの削除に失敗しました";
        return false;
    }

    ClearSelection();
    const bool wasSaved = uiLoadSystem->SaveCustomUI();
    statusMessage = wasSaved ? "選択中のUIを削除して保存しました" : "UIは削除しましたが保存に失敗しました";
    return true;
}

bool UICanvasEditorController::RestoreUndo(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    if (!uiLoadSystem) {
        return false;
    }
    std::string error;
    if (!mEditHistory.RestoreLatest(*uiLoadSystem, [&]() { return uiLoadSystem->SaveEditorUI(error); })) {
        if (!error.empty()) {
            statusMessage = "UI編集を元に戻せませんでした: " + error;
        }
        return false;
    }
    ClearSelection();
    statusMessage = "UI編集を元に戻して保存しました";
    return true;
}

void UICanvasEditorController::HandleShortcuts(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        SetOperation(Operation::Translate);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        SetOperation(Operation::Rotate);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
        SetOperation(Operation::Scale);
    }

    const bool isPrimaryModifierHeld = io.KeyCtrl || io.KeySuper;
    if (isPrimaryModifierHeld && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        DuplicateSelected(uiLoadSystem, statusMessage);
        return;
    }
    if (isPrimaryModifierHeld && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        RestoreUndo(uiLoadSystem, statusMessage);
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
        DeleteSelected(uiLoadSystem, statusMessage);
    }
}

void UICanvasEditorController::PushUndo(const UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    mEditHistory.Capture(*uiLoadSystem);
}
