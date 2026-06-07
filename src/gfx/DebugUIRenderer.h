#pragma once
#include "actor/Planet.h"
#include "imgui.h"
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

class Game;
class Player;
class Enemy;
class UIRenderer;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);

    void Draw();

private:
    void DrawPerformance();
    void DrawPlayer();
    void DrawEnemies();
    void DrawCamera();
    void DrawStagePlacement();
    void DrawUI();
    // void DrawStage1();
    void DrawDebugDrawSettings();
    void DrawStage();
    void DrawPlanets();
    void SaveStagePlanetsYaml();
    void UpdateActorsOnPlanetSurface(Planet* planet);
    void SavePlayerYaml(Player* player);
    void SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);
    void SaveStagePlacementYaml();
    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);
    std::string GetUIDisplayName(const std::string& key) const;

    template <typename T>
    bool SetYamlSequenceValue(YAML::Node& config, const std::string& sequenceName, std::size_t index,
                              const std::string& key, const T& value)
    {
        if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
            std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
            return false;
        }

        if (index >= config[sequenceName].size()) {
            std::cerr << "Index out of range: " << index << std::endl;
            return false;
        }

        config[sequenceName][index][key] = value;
        return true;
    }

    template <typename T>
    void DrawSphericalActorList(const std::string& label, const std::string& sequenceName,
                                const std::vector<T*>& actors)
    {
        const std::string treeLabel = label + "##" + sequenceName;

        if (!ImGui::TreeNode(treeLabel.c_str())) {
            return;
        }

        for (std::size_t i = 0; i < actors.size(); ++i) {
            T* actor = actors[i];
            if (!actor) {
                continue;
            }

            std::string itemLabel = label + " " + std::to_string(i) + "##" + sequenceName + std::to_string(i);

            if (ImGui::TreeNode(itemLabel.c_str())) {
                float theta = actor->GetTheta();
                float phi = actor->GetPhi();
                float height = actor->GetHeight();

                bool changed = false;

                changed |= ImGui::SliderFloat(("theta##" + sequenceName + std::to_string(i)).c_str(), &theta,
                                              -3.141593f, 3.141593f, "%.6f");

                changed |= ImGui::SliderFloat(("phi##" + sequenceName + std::to_string(i)).c_str(), &phi, -1.570796f,
                                              1.570796f, "%.6f");

                changed |= ImGui::SliderFloat(("height##" + sequenceName + std::to_string(i)).c_str(), &height, -2.0f,
                                              10.0f, "%.3f");

                if (changed) {
                    theta = std::round(theta * 1000000.0f) / 1000000.0f;
                    phi = std::round(phi * 1000000.0f) / 1000000.0f;
                    height = std::round(height * 1000.0f) / 1000.0f;

                    actor->SetSphericalPlacement(theta, phi, height);

                    Planet* planet = actor->GetCurrentPlanet();
                    if (planet) {
                        actor->SetPos(planet->CalculateSurfacePos(theta, phi, height));
                    }
                }

                const glm::vec3 pos = actor->GetPos();
                ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                ImGui::Text("yaml index: %d", actor->GetStageYamlIndex());

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    template <typename T>
    void SaveSphericalActors(YAML::Node& config, const std::string& sequenceName, const std::vector<T*>& actors)
    {
        for (T* actor : actors) {
            if (!actor) {
                continue;
            }

            const int index = actor->GetStageYamlIndex();
            if (index < 0) {
                continue;
            }

            SetYamlSequenceValue(config, sequenceName, static_cast<std::size_t>(index), "theta", actor->GetTheta());
            SetYamlSequenceValue(config, sequenceName, static_cast<std::size_t>(index), "phi", actor->GetPhi());
            SetYamlSequenceValue(config, sequenceName, static_cast<std::size_t>(index), "height", actor->GetHeight());
        }
    }

    Game* mGame;
    UIRenderer* mUIRenderer;
};