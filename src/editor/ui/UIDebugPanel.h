#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/ui/UICanvasEditorController.h"
#include "system/UILoadSystem.h"

#include <array>
#include <string>

// UI定義の一覧とプロパティを編集し、キャンバス操作をUICanvasEditorControllerへ委譲する。
class UIDebugPanel : public DebugPanel {
public:
    explicit UIDebugPanel(DebugEditorContext& context);

    // UI定義を直接編集する。明示的な保存操作に加え、キャンバスの複製・削除・ギズモ操作終了時にも保存する。
    void Draw() override;

private:
    enum class SelectedElementSource {
        None,
        Custom,
        ExistingTexture,
        ExistingText,
    };

    void DrawUIEditor(UILoadSystem* uiLoadSystem);
    void DrawCanvasToolbar();
    void DrawElementList(UILoadSystem* uiLoadSystem);
    void DrawElementInspector(UILoadSystem* uiLoadSystem);
    void DrawCustomElementInspector(UILoadSystem* uiLoadSystem);
    void DrawExistingTextureInspector(UILoadSystem* uiLoadSystem);
    void DrawExistingTextInspector(UILoadSystem* uiLoadSystem);
    void DrawCodeBoundElementProtection();
    void DrawAssetPicker(std::string& texturePath, bool& flipVertical, const char* widgetId, const char* sectionLabel);
    void SaveAllUI(UILoadSystem* uiLoadSystem);
    void ReloadAllUI(UILoadSystem* uiLoadSystem);

    std::string GetDisplayName(const std::string& key) const;

    UICanvasEditorController mCanvasEditor;
    int mNewElementType = 0;
    std::array<char, 128> mNewScreen = {"custom"};
    std::array<char, 128> mNewId = {"element"};
    std::array<char, 128> mAssetFilter = {};
    int mSelectedInputDeviceImageVariant = 0;
    SelectedElementSource mSelectedElementSource = SelectedElementSource::None;
    std::string mSelectedExistingElementKey;
    std::string mStatusMessage;
};
