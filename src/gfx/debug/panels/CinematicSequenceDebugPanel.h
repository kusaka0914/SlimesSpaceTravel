#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class CinematicSequenceDebugPanel : public DebugPanel {
public:
    explicit CinematicSequenceDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void SelectSequence(const std::string& sequenceId);
    void SelectKeyframe(int keyframeIndex);

private:
    char mNewSequenceIdBuffer[128] = "new_sequence";
    char mRenameSequenceIdBuffer[128] = {};

    std::string mSelectedSequenceId;
    std::string mStatusMessage;

    int mSelectedKeyframeIndex = -1;
    int mEasingIndex = 3;
    int mTransitionModeIndex = 0;

    float mKeyframeTime = 0.0f;
    float mKeyframeHoldDurationSeconds = 0.0f;
};
