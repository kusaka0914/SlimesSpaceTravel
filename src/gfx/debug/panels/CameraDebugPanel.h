#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class CameraDebugPanel : public DebugPanel {
public:
    explicit CameraDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    std::string mStatusMessage;
};
