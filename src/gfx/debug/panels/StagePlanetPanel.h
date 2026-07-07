#pragma once

#include "gfx/debug/DebugPanel.h"

class Actor;
class Planet;

class StagePlanetPanel : public DebugPanel {
public:
    explicit StagePlanetPanel(DebugEditorContext& context);

    void Draw() override;
    void Save();

private:
    void UpdateActorsOnPlanetSurface(Planet* planet);
};