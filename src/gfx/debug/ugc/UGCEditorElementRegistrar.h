#pragma once

#include "gfx/debug/DebugEditorContext.h"

class UGCEditorElementRegistrar {
public:
    explicit UGCEditorElementRegistrar(DebugEditorContext& context);

    void RegisterElements();

private:
    DebugEditorContext& mContext;
};

