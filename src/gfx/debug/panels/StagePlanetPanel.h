#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <string>
#include <vector>

class Actor;
class Planet;

class StagePlanetPanel : public DebugPanel {
public:
    explicit StagePlanetPanel(DebugEditorContext& context);

    void Draw() override;
    void DrawSelectedPlanet(Planet* selectedPlanet);
    void Save();

private:
    void DrawTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawBackTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex);
    void UpdateActorsOnPlanetSurface(Planet* planet);

private:
    Planet* mFocusedPlanet = nullptr;
    std::array<char, 128> mTextureAssetFilter = {};
    std::string mTextureAssetStatus;
};
