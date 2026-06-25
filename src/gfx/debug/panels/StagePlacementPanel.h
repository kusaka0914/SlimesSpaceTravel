#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "imgui.h"
#include "system/PhysicsSystem.h"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class StagePlacementPanel : public DebugPanel {
public:
    StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController);

    void Draw() override;
    void Save();

    void RequestOpenPickedActorPlacement();

private:
    void SavePlatformsYaml(YAML::Node& config, const std::vector<Platform*>& platforms);

    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);

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

        const auto& pickedActorRef = mSelectionController.GetPickedActorRef();
        if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == sequenceName) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }

        if (!ImGui::TreeNode(treeLabel.c_str())) {
            return;
        }

        for (std::size_t i = 0; i < actors.size(); ++i) {
            T* actor = actors[i];
            if (!actor) {
                continue;
            }

            const int yamlIndex = actor->GetStageYamlIndex();

            std::string itemLabel = label + " " + std::to_string(i) + "##" + sequenceName + std::to_string(i);

            if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == sequenceName &&
                pickedActorRef->yamlIndex == yamlIndex) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            }

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

                    if (Platform* platform = dynamic_cast<Platform*>(actor)) {
                        ApplyActorEditorRotation(platform);
                    }
                }

                bool posChanged = false;
                bool physicsRebuildRequired = false;

                Planet* planet = actor->GetCurrentPlanet();

                glm::vec3 localPos = actor->GetPos();
                if (planet) {
                    localPos -= planet->GetPos();
                }

                posChanged |= ImGui::DragFloat(("posX##actorPosX" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.x, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                posChanged |= ImGui::DragFloat(("posY##actorPosY" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.y, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                posChanged |= ImGui::DragFloat(("posZ##actorPosZ" + sequenceName + std::to_string(i)).c_str(),
                                               &localPos.z, 0.01f, -100.0f, 100.0f, "%.2f");
                physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

                if (posChanged) {
                    localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
                    localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
                    localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

                    const glm::vec3 worldPos = planet ? planet->GetPos() + localPos : localPos;
                    actor->SetPos(worldPos);
                }

                if (Platform* platform = dynamic_cast<Platform*>(actor)) {
                    glm::vec3 rotationRad = platform->GetEditorRotation();
                    glm::vec3 rotationDeg = glm::degrees(rotationRad);

                    bool rotationChanged = false;

                    rotationChanged |=
                        ImGui::DragFloat(("Pitch##platformPitch" + sequenceName + std::to_string(i)).c_str(),
                                         &rotationDeg.x, 0.1f, -180.0f, 180.0f, "%.1f");

                    rotationChanged |= ImGui::DragFloat(("Yaw##platformYaw" + sequenceName + std::to_string(i)).c_str(),
                                                        &rotationDeg.y, 0.1f, -180.0f, 180.0f, "%.1f");

                    rotationChanged |=
                        ImGui::DragFloat(("Roll##platformRoll" + sequenceName + std::to_string(i)).c_str(),
                                         &rotationDeg.z, 0.1f, -180.0f, 180.0f, "%.1f");

                    if (rotationChanged) {
                        rotationDeg.x = std::round(rotationDeg.x * 10.0f) / 10.0f;
                        rotationDeg.y = std::round(rotationDeg.y * 10.0f) / 10.0f;
                        rotationDeg.z = std::round(rotationDeg.z * 10.0f) / 10.0f;

                        rotationRad = glm::radians(rotationDeg);

                        platform->SetEditorRotation(rotationRad);
                        ApplyActorEditorRotation(platform);

                        physicsRebuildRequired = true;
                    }

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

                    if (physicsRebuildRequired && mContext.game && mContext.game->GetPhysicsSystem()) {
                        mContext.game->GetPhysicsSystem()->Initialize();
                    }
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
        for (T* actor : actors) {
            if (!actor) {
                continue;
            }

            const int index = actor->GetStageYamlIndex();
            if (index < 0) {
                continue;
            }

            const std::size_t yamlIndex = static_cast<std::size_t>(index);

            SetYamlSequenceValue(config, sequenceName, yamlIndex, "theta", actor->GetTheta());
            SetYamlSequenceValue(config, sequenceName, yamlIndex, "phi", actor->GetPhi());
            SetYamlSequenceValue(config, sequenceName, yamlIndex, "height", actor->GetHeight());

            glm::vec3 localPos = actor->GetPos();
            if (actor->GetCurrentPlanet()) {
                localPos -= actor->GetCurrentPlanet()->GetPos();
            }

            localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
            localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
            localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

            SetYamlSequenceValue(config, sequenceName, yamlIndex, "pos", YAML::Node(YAML::NodeType::Sequence));
            config[sequenceName][yamlIndex]["pos"][0] = localPos.x;
            config[sequenceName][yamlIndex]["pos"][1] = localPos.y;
            config[sequenceName][yamlIndex]["pos"][2] = localPos.z;
        }
    }

private:
    StageSelectionController& mSelectionController;
    bool mRequestOpenPickedActorPlacement = false;
};