#pragma once

#include <GL/glew.h>

#include "Game.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "imgui.h"
#include "system/PhysicsSystem.h"
#include <cmath>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Game;
class Player;
class Enemy;
class UIRenderer;
class UILoadSystem;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);

    void Draw();

private:
    void DrawPerformance();
    void DrawCamera();
    void DrawParameterEditor();
    void AddEnemyFromEditor(const std::string& type, int currentPlanetNum);
    void DrawPlayerParameterEditor();
    void DrawEnemyParameterEditor();
    void DrawParameterSave();
    void DrawStagePlacement();
    void DrawUI();
    void DrawUITextures(UILoadSystem* uiLoadSystem);
    void DrawUITexts(UILoadSystem* uiLoadSystem);
    void DrawPlanets();
    void DrawStageEditor();
    void DrawAddActors();
    void SaveStagePlanetsYaml();
    void UpdateActorsOnPlanetSurface(Planet* planet);
    void AddPlanetFromEditor(const std::string& modelPath);
    void SavePlayerYaml(Player* player);
    void SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);
    void SaveStagePlacementYaml();
    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);
    std::string GetUIDisplayName(const std::string& key) const;
    void AddPlatformFromEditor(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale);

    void SavePlatformsYaml(YAML::Node& config, const std::vector<Platform*>& platforms);

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

                changed |= ImGui::DragFloat(("theta##" + sequenceName + std::to_string(i)).c_str(), &theta, 0.001f,
                                            -3.141593f, 3.141593f, "%.6f");

                changed |= ImGui::DragFloat(("phi##" + sequenceName + std::to_string(i)).c_str(), &phi, 0.001f,
                                            -1.570796f, 1.570796f, "%.6f");

                changed |= ImGui::DragFloat(("height##" + sequenceName + std::to_string(i)).c_str(), &height, 0.01f,
                                            -10.0f, 10.0f, "%.3f");

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

                if (Platform* platform = dynamic_cast<Platform*>(actor)) {
                    glm::vec3 scale = platform->GetScale();

                    bool scaleChanged = false;

                    bool physicsRebuildRequired = false;

                    scaleChanged |=
                        ImGui::DragFloat(("スケールX##platformScaleX" + sequenceName + std::to_string(i)).c_str(),
                                         &scale.x, 0.01f, 0.1f, 30.0f, "%.2f");
                    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                    scaleChanged |=
                        ImGui::DragFloat(("スケールY##platformScaleY" + sequenceName + std::to_string(i)).c_str(),
                                         &scale.y, 0.01f, 0.1f, 30.0f, "%.2f");
                    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                    scaleChanged |=
                        ImGui::DragFloat(("スケールZ##platformScaleZ" + sequenceName + std::to_string(i)).c_str(),
                                         &scale.z, 0.01f, 0.1f, 30.0f, "%.2f");
                    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                    if (scaleChanged) {
                        scale.x = std::round(scale.x * 100.0f) / 100.0f;
                        scale.y = std::round(scale.y * 100.0f) / 100.0f;
                        scale.z = std::round(scale.z * 100.0f) / 100.0f;

                        platform->SetScale(scale);
                    }

                    float facingYaw = actor->GetFacingYaw();
                    if (ImGui::SliderFloat("向き", &facingYaw, -3.14159f, 3.14159f, "%.3f")) {
                        platform->SetFacingYaw(facingYaw);
                    }
                    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                    if (physicsRebuildRequired && mGame->GetPhysicsSystem()) {
                        mGame->GetPhysicsSystem()->Initialize();
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