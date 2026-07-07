#pragma once

#include "gfx/debug/DebugPanel.h"

class CameraDebugPanel : public DebugPanel {
public:
    explicit CameraDebugPanel(DebugEditorContext& context);

    void Draw() override;
};