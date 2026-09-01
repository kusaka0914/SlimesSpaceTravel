#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>

class DebugBuildRestartPanel {
public:
    explicit DebugBuildRestartPanel(DebugEditorContext& context);

    void Draw();
    void SetStatus(const std::string& message, bool isError);

private:
    DebugEditorContext& mContext;
    std::string mStatusMessage;
    bool mIsStatusError = false;
};

