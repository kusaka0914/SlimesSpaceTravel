#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class CameraDebugPanel : public DebugPanel {
public:
    explicit CameraDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void SelectSequence(const std::string& sequenceId);
    void SelectKeyframe(int keyframeIndex);

private:
    char mSequenceIdBuffer[128] = "new_sequence";

    std::string mSelectedSequenceId;
    std::string mStatusMessage;

    int mSelectedKeyframeIndex = -1;
    int mEasingIndex = 3;

    float mKeyframeTime = 0.0f;
};
