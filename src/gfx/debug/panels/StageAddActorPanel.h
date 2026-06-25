#pragma once

#include "gfx/debug/DebugPanel.h"

#include <glm/glm.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class StageAddActorPanel : public DebugPanel {
public:
    explicit StageAddActorPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawPlanetCombo(const char* label, int& selectedPlanetIndex);

    void AddPlatformFromEditor(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale);
    void AddPlanetFromEditor(const std::string& modelPath);
    void AddEnemyFromEditor(const std::string& type, int currentPlanetNum);
    void AddNPCFromEditor(const std::string& type, int currentPlanetNum);
    void AddCrystalFromEditor(const std::string& type, int currentPlanetNum);
    void AddBoatPartsFromEditor(const std::string& type, int currentPlanetNum);
    void AddBoatFromEditor(int startPlanetNum, int destPlanetNum, int destStage);
    void AddStarFromEditor(int currentPlanetNum);

    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);

private:
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
};