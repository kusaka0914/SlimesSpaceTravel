#pragma once

#include "gfx/debug/DebugPanel.h"
#include "system/ending/EndingRollConfig.h"

#include <string>

class EndingRollDebugPanel : public DebugPanel {
public:
    explicit EndingRollDebugPanel(DebugEditorContext& context);
    void Draw() override;

private:
    void DrawSettings();
    void DrawPreview();
    void DrawImagePicker(const char* label, std::string& imagePath);
    void Reload();

    EndingRollConfig mConfig;
    std::string mStatus;
    float mPreviewTime = 0.0f;
    bool mIsPreviewPlaying = false;
    int mSelectedImageIndex = -1;
};
