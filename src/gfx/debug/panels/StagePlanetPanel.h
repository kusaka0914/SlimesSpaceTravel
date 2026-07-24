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
    void Save();

private:
    void DrawTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex);
    void RefreshTextureAssets();
    void UpdateActorsOnPlanetSurface(Planet* planet);

private:
    std::array<char, 128> mTextureAssetFilter = {};
    std::vector<std::string> mTextureAssets;
    std::string mTextureAssetStatus;
    bool mTextureAssetsScanned = false;
};
