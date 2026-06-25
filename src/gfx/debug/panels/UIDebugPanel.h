#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class UILoadSystem;

class UIDebugPanel : public DebugPanel {
public:
    explicit UIDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawTextures(UILoadSystem* uiLoadSystem);
    void DrawTexts(UILoadSystem* uiLoadSystem);

    std::string GetDisplayName(const std::string& key) const;

private:
    int mSelectedMenu = 0;
};