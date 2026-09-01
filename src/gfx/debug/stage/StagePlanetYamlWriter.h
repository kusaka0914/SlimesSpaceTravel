#pragma once

#include "gfx/debug/DebugEditorContext.h"

class StagePlanetYamlWriter {
public:
    explicit StagePlanetYamlWriter(DebugEditorContext& context);

    bool Save(bool shouldSaveEditorTransform) const;

private:
    DebugEditorContext& mContext;
};
