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
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

class Game;
class Player;
class Enemy;
class UIRenderer;
class UILoadSystem;

class DebugUIRenderer {
public:
    enum class DeleteActorType { Enemy, Platform, Crystal, NPC, BoatParts, Boat, Key, Star };

    struct DeleteTargetInfo {
        DeleteActorType type;
        int yamlIndex = -1;
        std::string sequenceName;
        std::string label;
    };

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
    void DrawPlanetCombo(const char* label, int& selectedPlanetIndex);
    void SaveStagePlanetsYaml();
    void UpdateActorsOnPlanetSurface(Planet* planet);
    void AddPlanetFromEditor(const std::string& modelPath);
    void AddNPCFromEditor(const std::string& type, int currentPlanetNum);
    void AddCrystalFromEditor(const std::string& type, int currentPlanetNum);
    void AddBoatPartsFromEditor(const std::string& type, int currentPlanetNum);
    void AddBoatFromEditor(int startPlanetNum, int destPlanetNum, int destStage);
    void AddStarFromEditor(int currentPlanetNum);
    void SavePlayerYaml(Player* player);
    void SaveEnemiesYaml(Enemy* normalEnemy, Enemy* bossEnemy);
    void SaveStagePlacementYaml();
    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);
    std::string GetUIDisplayName(const std::string& key) const;
    void AddPlatformFromEditor(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale);
    void DrawDeleteActors();
    void DeleteSelectedActorsFromEditor(const std::vector<DeleteTargetInfo>& targets,
                                        const std::unordered_set<std::string>& selectedKeys);

    std::string GetDeleteSequenceName(DeleteActorType type) const;
    const char* GetDeleteTypeLabel(DeleteActorType type) const;

    std::vector<DeleteTargetInfo> CollectAllDeleteTargets() const;

    void DeleteActorFromEditor(DeleteActorType type, int yamlIndex);
    bool RemoveYamlSequenceElement(YAML::Node& config, const std::string& sequenceName, int index);

    void SavePlatformsYaml(YAML::Node& config, const std::vector<Platform*>& platforms);
    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const;
    void ApplyActorEditorRotation(Actor* actor);

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

                bool physicsRebuildRequired = false;

                bool posChanged = false;

                Planet* planet = actor->GetCurrentPlanet();

                glm::vec3 localPos = actor->GetPos();
                if (planet) {
                    localPos -= planet->GetPos();
                }

                posChanged |= ImGui::DragFloat(("posX##platformPosX" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.x, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                posChanged |= ImGui::DragFloat(("posY##platformPosY" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.y, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                posChanged |= ImGui::DragFloat(("posZ##platformPosZ" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.z, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                if (posChanged) {
                    localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
                    localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
                    localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

                    const glm::vec3 worldPos = planet ? planet->GetPos() + localPos : localPos;
                    actor->SetPos(worldPos);
                }

                glm::vec3 rotationRad = actor->GetEditorRotation();
                glm::vec3 rotationDeg = glm::degrees(rotationRad);

                bool rotationChanged = false;

                rotationChanged |= ImGui::DragFloat(("Pitch##actorPitch" + sequenceName + std::to_string(i)).c_str(),
                                                    &rotationDeg.x, 0.1f, -180.0f, 180.0f, "%.1f");

                rotationChanged |= ImGui::DragFloat(("Yaw##actorYaw" + sequenceName + std::to_string(i)).c_str(),
                                                    &rotationDeg.y, 0.1f, -180.0f, 180.0f, "%.1f");

                rotationChanged |= ImGui::DragFloat(("Roll##actorRoll" + sequenceName + std::to_string(i)).c_str(),
                                                    &rotationDeg.z, 0.1f, -180.0f, 180.0f, "%.1f");

                if (rotationChanged) {
                    rotationDeg.x = std::round(rotationDeg.x * 10.0f) / 10.0f;
                    rotationDeg.y = std::round(rotationDeg.y * 10.0f) / 10.0f;
                    rotationDeg.z = std::round(rotationDeg.z * 10.0f) / 10.0f;

                    rotationRad = glm::radians(rotationDeg);

                    actor->SetEditorRotation(rotationRad);
                    ApplyActorEditorRotation(actor);

                    physicsRebuildRequired = true;
                }

                if (changed) {
                    theta = std::round(theta * 1000000.0f) / 1000000.0f;
                    phi = std::round(phi * 1000000.0f) / 1000000.0f;
                    height = std::round(height * 1000.0f) / 1000.0f;

                    actor->SetSphericalPlacement(theta, phi, height);

                    Planet* planet = actor->GetCurrentPlanet();
                    if (planet) {
                        actor->SetPos(planet->CalculateSurfacePos(theta, phi, height));
                    }

                    ApplyActorEditorRotation(actor);
                }

                if (Platform* platform = dynamic_cast<Platform*>(actor)) {
                    bool physicsRebuildRequired = false;

                    glm::vec3 scale = platform->GetScale();

                    bool scaleChanged = false;

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

                    // if (physicsRebuildRequired && mGame->GetPhysicsSystem()) {
                    //     mGame->GetPhysicsSystem()->Initialize();
                    // }
                }

                const glm::vec3 pos = actor->GetPos();
                ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    template <typename T>
    void SaveSphericalActors(YAML::Node& config, const std::string& sequenceName, const std::vector<T*>& actors)
    {
        YAML::Node newSequence(YAML::NodeType::Sequence);
        newSequence.SetStyle(YAML::EmitterStyle::Block);

        for (T* actor : actors) {
            if (!actor) {
                continue;
            }

            YAML::Node oldNode;

            const int index = actor->GetStageYamlIndex();
            if (index >= 0 && config[sequenceName] && config[sequenceName].IsSequence() &&
                static_cast<std::size_t>(index) < config[sequenceName].size()) {
                oldNode = config[sequenceName][static_cast<std::size_t>(index)];
            }

            YAML::Node node(YAML::NodeType::Map);
            node.SetStyle(YAML::EmitterStyle::Block);

            // 既存情報を残す
            if (oldNode) {
                for (YAML::const_iterator it = oldNode.begin(); it != oldNode.end(); ++it) {
                    node[it->first.as<std::string>()] = it->second;
                }
            }

            node["theta"] = actor->GetTheta();
            node["phi"] = actor->GetPhi();
            node["height"] = actor->GetHeight();

            glm::vec3 localPos = actor->GetPos();

            if (actor->GetCurrentPlanet()) {
                localPos -= actor->GetCurrentPlanet()->GetPos();
            }

            localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
            localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
            localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

            YAML::Node posNode(YAML::NodeType::Sequence);
            posNode.SetStyle(YAML::EmitterStyle::Block);
            posNode.push_back(localPos.x);
            posNode.push_back(localPos.y);
            posNode.push_back(localPos.z);

            node["pos"] = posNode;

            const glm::vec3 rotation = actor->GetEditorRotation();

            YAML::Node rotationNode(YAML::NodeType::Sequence);
            rotationNode.SetStyle(YAML::EmitterStyle::Block);
            rotationNode.push_back(rotation.x);
            rotationNode.push_back(rotation.y);
            rotationNode.push_back(rotation.z);

            node["rotation"] = rotationNode;
            node["facingYaw"] = rotation.y;

            const glm::vec3 upVec = actor->GetUpVec();

            YAML::Node upVecNode(YAML::NodeType::Sequence);
            upVecNode.SetStyle(YAML::EmitterStyle::Block);
            upVecNode.push_back(upVec.x);
            upVecNode.push_back(upVec.y);
            upVecNode.push_back(upVec.z);

            node["upVec"] = upVecNode;

            newSequence.push_back(node);
        }

        config[sequenceName] = newSequence;
    }

    Game* mGame;
    UIRenderer* mUIRenderer;
};