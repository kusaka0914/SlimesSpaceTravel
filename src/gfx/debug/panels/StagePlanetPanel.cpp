#include "gfx/debug/panels/StagePlanetPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

StagePlanetPanel::StagePlanetPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void StagePlanetPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (!ImGui::TreeNode("惑星")) {
        return;
    }

    ImGui::Separator();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const std::string treeLabel = "惑星 " + std::to_string(i) + "##planet" + std::to_string(i);

        if (ImGui::TreeNode(treeLabel.c_str())) {
            glm::vec3 center = planet->GetPos();
            glm::vec3 scale = planet->GetScale();

            bool centerChanged = false;
            bool scaleChanged = false;

            centerChanged |= ImGui::SliderFloat(("中心X##planetCenterX" + std::to_string(i)).c_str(), &center.x,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Y##planetCenterY" + std::to_string(i)).c_str(), &center.y,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Z##planetCenterZ" + std::to_string(i)).c_str(), &center.z,
                                                -100.0f, 100.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールX##planetScaleX" + std::to_string(i)).c_str(), &scale.x, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールY##planetScaleY" + std::to_string(i)).c_str(), &scale.y, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールZ##planetScaleZ" + std::to_string(i)).c_str(), &scale.z, 0.1f,
                                               30.0f, "%.2f");

            if (centerChanged) {
                planet->SetPos(center);
                UpdateActorsOnPlanetSurface(planet);
            }

            if (scaleChanged) {
                bool isSphere = false;

                scale.x = std::round(scale.x * 100.0f) / 100.0f;
                scale.y = std::round(scale.y * 100.0f) / 100.0f;
                scale.z = std::round(scale.z * 100.0f) / 100.0f;

                if (scale.x == scale.y && scale.y == scale.z && scale.x == scale.z) {
                    isSphere = true;
                }

                planet->SetScale(scale);

                if (isSphere) {
                    planet->SetPlanetShape("Sphere");
                } else {
                    planet->SetPlanetShape("Ellipse");
                }

                planet->SetRadius(scale.x);

                UpdateActorsOnPlanetSurface(planet);
            }

            const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
            const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

            std::string currentModel = planet->GetModelPath();
            int selectedModelIndex = 0;

            for (int modelIndex = 0; modelIndex < IM_ARRAYSIZE(planetModels); ++modelIndex) {
                if (currentModel == planetModels[modelIndex]) {
                    selectedModelIndex = modelIndex;
                    break;
                }
            }

            if (ImGui::Combo(("モデル##planetModel" + std::to_string(i)).c_str(), &selectedModelIndex,
                             planetModelLabels, IM_ARRAYSIZE(planetModelLabels))) {
                planet->SetModelPath(planetModels[selectedModelIndex]);

                if (mContext.game->GetMeshLoadSystem()) {
                    mContext.game->GetMeshLoadSystem()->SetActorMesh(planet);
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
}

void StagePlanetPanel::Save()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const glm::vec3 center = planet->GetPos();
        const glm::vec3 scale = planet->GetScale();

        config["planets"][i]["center"][0] = center.x;
        config["planets"][i]["center"][1] = center.y;
        config["planets"][i]["center"][2] = center.z;

        config["planets"][i]["scale"][0] = scale.x;
        config["planets"][i]["scale"][1] = scale.y;
        config["planets"][i]["scale"][2] = scale.z;

        config["planets"][i]["model"] = planet->GetModelPath();
    }

    SaveYamlFile(filePath, config);
}

void StagePlanetPanel::UpdateActorsOnPlanetSurface(Planet* planet)
{
    if (!planet || !mContext.game) {
        return;
    }

    auto updateActor = [planet](Actor* actor) {
        if (!actor) {
            return;
        }

        const glm::vec3 newPos = planet->CalculateSurfacePos(actor->GetTheta(), actor->GetPhi(), actor->GetHeight());

        actor->SetPos(newPos);
    };

    if (!mContext.game->GetPlayers().empty()) {
        updateActor(mContext.game->GetPlayers()[0]);
    }

    for (Enemy* enemy : planet->GetEnemies()) {
        updateActor(enemy);
    }

    for (Crystal* crystal : planet->GetCrystals()) {
        updateActor(crystal);
    }

    for (Boat* boat : planet->GetBoats()) {
        updateActor(boat);
    }

    for (BoatParts* part : planet->GetBoatParts()) {
        updateActor(part);
    }

    for (NPC* npc : planet->GetNPCs()) {
        updateActor(npc);
    }

    if (Key* key = planet->GetKey()) {
        updateActor(key);
    }

    if (Star* star = planet->GetStar()) {
        updateActor(star);
    }
}

bool StagePlanetPanel::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}