#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageActorCreateService.h"

#include <array>
#include <glm/glm.hpp>
#include <string>

class StageAddActorPanel : public DebugPanel {
public:
    explicit StageAddActorPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawPlanetCombo(const char* label, int& selectedPlanetIndex);

private:
    StageActorCreateService mCreateService;

    int mSelectedPlanetModelIndex = 0;

    int mSelectedEnemyTypeIndex = 0;
    int mSelectedEnemyPlanetIndex = -1;

    int mSelectedPlatformPlanetIndex = -1;
    int mSelectedPlatformModelIndex = 0;
    glm::vec3 mPlatformScale = glm::vec3(1.0f, 1.0f, 1.0f);

    int mSelectedCrystalPlanetIndex = -1;
    int mSelectedCrystalTypeIndex = 0;

    int mSelectedNPCPlanetIndex = -1;
    int mSelectedNPCTypeIndex = 0;

    int mSelectedBoatPartsPlanetIndex = -1;
    int mSelectedBoatPartsTypeIndex = 0;

    int mSelectedBoatStartPlanetIndex = -1;
    int mSelectedBoatDestPlanetIndex = -1;
    int mSelectedBoatDestStage = 0;

    int mSelectedStarPlanetIndex = -1;

    int mSelectedStageObjectPlanetIndex = -1;
    std::string mSelectedStageObjectModel;
    std::array<char, 128> mStageObjectSearch = {};
    bool mStageObjectCollisionEnabled = true;
    std::string mStageObjectStatus;
};
