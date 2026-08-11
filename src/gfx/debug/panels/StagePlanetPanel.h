#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <functional>
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
    void SaveEditorAuthoredTransforms();
    void SetSaveDependentActorTransformsCallback(
        std::function<void()> callback);

private:
    bool SaveYaml(bool shouldSaveEditorTransform);
    void DrawTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawBackTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex);
private:
    Planet* mFocusedPlanet = nullptr;
    std::array<char, 128> mTextureAssetFilter = {};
    std::string mTextureAssetStatus;
    bool mHasPendingTransformEdit = false;
    std::function<void()> mSaveDependentActorTransformsCallback;
};
