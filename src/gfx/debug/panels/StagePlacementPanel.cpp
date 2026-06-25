#include "gfx/debug/panels/StagePlacementPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Star.h"

#include <fstream>
#include <iostream>

StagePlacementPanel::StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController)
    : DebugPanel(context),
      mSelectionController(selectionController)
{
}

void StagePlacementPanel::RequestOpenPickedActorPlacement()
{
    mRequestOpenPickedActorPlacement = true;
}

void StagePlacementPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (mRequestOpenPickedActorPlacement) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode("オブジェクト配置")) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    std::vector<Enemy*> enemies;
    std::vector<Crystal*> crystals;
    std::vector<Boat*> boats;
    std::vector<BoatParts*> boatParts;
    std::vector<NPC*> npcs;
    std::vector<Key*> keys;
    std::vector<Platform*> platforms;
    std::vector<Star*> stars;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            enemies.emplace_back(enemy);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            crystals.emplace_back(crystal);
        }

        for (Boat* boat : planet->GetBoats()) {
            boats.emplace_back(boat);
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            boatParts.emplace_back(part);
        }

        for (NPC* npc : planet->GetNPCs()) {
            npcs.emplace_back(npc);
        }

        if (Key* key = planet->GetKey()) {
            keys.emplace_back(key);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            platforms.emplace_back(platform);
        }

        if (Star* star = planet->GetStar()) {
            stars.emplace_back(star);
        }
    }

    ImGui::Separator();

    DrawSphericalActorList("敵", "enemies", enemies);
    DrawSphericalActorList("足場", "platforms", platforms);
    DrawSphericalActorList("キー", "keys", keys);
    DrawSphericalActorList("ボート", "boats", boats);
    DrawSphericalActorList("ボートパーツ", "boatParts", boatParts);
    DrawSphericalActorList("クリスタル", "crystals", crystals);
    DrawSphericalActorList("NPC", "NPCs", npcs);
    DrawSphericalActorList("星", "star", stars);

    ImGui::TreePop();

    mRequestOpenPickedActorPlacement = false;
}

void StagePlacementPanel::Save()
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

    std::vector<Enemy*> enemies;
    std::vector<Crystal*> crystals;
    std::vector<Boat*> boats;
    std::vector<BoatParts*> boatParts;
    std::vector<NPC*> npcs;
    std::vector<Key*> keys;
    std::vector<Platform*> platforms;
    std::vector<Star*> stars;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            enemies.emplace_back(enemy);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            crystals.emplace_back(crystal);
        }

        for (Boat* boat : planet->GetBoats()) {
            boats.emplace_back(boat);
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            boatParts.emplace_back(part);
        }

        for (NPC* npc : planet->GetNPCs()) {
            npcs.emplace_back(npc);
        }

        if (Key* key = planet->GetKey()) {
            keys.emplace_back(key);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            platforms.emplace_back(platform);
        }

        if (Star* star = planet->GetStar()) {
            stars.emplace_back(star);
        }
    }

    SaveSphericalActors(config, "enemies", enemies);
    SaveSphericalActors(config, "keys", keys);
    SaveSphericalActors(config, "boats", boats);
    SaveSphericalActors(config, "boatParts", boatParts);
    SaveSphericalActors(config, "crystals", crystals);
    SaveSphericalActors(config, "NPCs", npcs);
    SaveSphericalActors(config, "star", stars);
    SavePlatformsYaml(config, platforms);

    SaveYamlFile(filePath, config);
}

void StagePlacementPanel::SavePlatformsYaml(YAML::Node& config, const std::vector<Platform*>& platforms)
{
    config["platforms"] = YAML::Node(YAML::NodeType::Sequence);

    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    for (Platform* platform : platforms) {
        if (!platform) {
            continue;
        }

        int currentPlanetNum = 0;

        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            if (planets[i] == platform->GetCurrentPlanet()) {
                currentPlanetNum = i;
                break;
            }
        }

        const glm::vec3 scale = platform->GetScale();

        YAML::Node node;

        node["currentPlanetNum"] = currentPlanetNum;

        glm::vec3 localPos = platform->GetPos();
        if (platform->GetCurrentPlanet()) {
            localPos -= platform->GetCurrentPlanet()->GetPos();
        }

        localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
        localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
        localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

        node["pos"][0] = localPos.x;
        node["pos"][1] = localPos.y;
        node["pos"][2] = localPos.z;

        node["theta"] = platform->GetTheta();
        node["phi"] = platform->GetPhi();
        node["height"] = platform->GetHeight();

        node["facingYaw"] = platform->GetFacingYaw();

        const glm::vec3 rotation = platform->GetEditorRotation();

        node["rotation"][0] = rotation.x;
        node["rotation"][1] = rotation.y;
        node["rotation"][2] = rotation.z;

        node["scale"][0] = scale.x;
        node["scale"][1] = scale.y;
        node["scale"][2] = scale.z;

        node["modelPath"] = platform->GetModelPath();

        YAML::Node upVecNode;
        glm::vec3 upVec = platform->GetUpVec();

        upVecNode.push_back(upVec.x);
        upVecNode.push_back(upVec.y);
        upVecNode.push_back(upVec.z);

        node["upVec"] = upVecNode;

        config["platforms"].push_back(node);
    }
}

bool StagePlacementPanel::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

glm::vec3 StagePlacementPanel::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);

    Planet* planet = actor->GetCurrentPlanet();

    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        glm::vec3 toActor = actor->GetPos() - planet->GetPos();

        if (glm::length(toActor) > 1e-6f) {
            baseUp = glm::normalize(toActor);
        }
    }

    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);

    baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);

    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);
    }

    baseForward = glm::normalize(baseForward);

    glm::vec3 baseRight = glm::normalize(glm::cross(baseForward, baseUp));

    const float pitch = rotationRad.x;
    const float yaw = rotationRad.y;
    const float roll = rotationRad.z;

    glm::mat4 rot(1.0f);
    rot = glm::rotate(rot, yaw, baseUp);
    rot = glm::rotate(rot, pitch, baseRight);
    rot = glm::rotate(rot, roll, baseForward);

    glm::vec3 upVec = glm::vec3(rot * glm::vec4(baseUp, 0.0f));

    if (glm::length(upVec) < 1e-6f) {
        return baseUp;
    }

    return glm::normalize(upVec);
}

void StagePlacementPanel::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor) {
        return;
    }

    const glm::vec3 rotation = actor->GetEditorRotation();

    actor->SetFacingYaw(rotation.y);
    actor->SetUpVec(CalculateActorUpVecFromEditorRotation(actor, rotation));
}