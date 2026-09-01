#pragma once

#include "gfx/debug/DebugPanel.h"
#include "system/story/StorybookConfig.h"

#include <string>

class StorybookDebugPanel : public DebugPanel {
public:
    explicit StorybookDebugPanel(DebugEditorContext& context);
    void Draw() override;

private:
    void Reload();
    void DrawTrack(const char* trackId, const char* displayName, bool isOpening);
    void DrawImagePicker(std::string& imagePath);
    void NormalizeTrackImageCount(const char* trackId, bool isOpening);

    StorybookConfig mConfig;
    std::string mStatus;
    int mSelectedTrack = 0;
};
