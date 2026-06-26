#include "ActorLoadSystem.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Key.h"
#include "actor/MovingPlatform.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "system/MeshLoadSystem.h"
#include <glm/glm.hpp>
#include <iostream>

ActorLoadSystem::ActorLoadSystem(Game* game)
    : mGame(game)
{
}

void ActorLoadSystem::LoadData(bool isLoadPlayer)
{
    const std::string& path = mGame->GetCurrentStageYamlPath();

    LoadPlanets(path.c_str());
    LoadEnemies(path.c_str());
    LoadBoatArrivalPoints(path.c_str());
    LoadBoats(path.c_str());
    LoadBoatParts(path.c_str());
    LoadKeys(path.c_str());
    LoadCrystals(path.c_str());
    LoadStar(path.c_str());
    LoadNPCs(path.c_str());
    LoadPlatforms(path.c_str());
    LoadMovingPlatforms(path.c_str());
    LoadFallRespawnPoints(path.c_str());
    LoadPlayers(path.c_str());
}

void ActorLoadSystem::LoadPlayers(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["players"] || !root["players"].IsSequence()) {
        return;
    }

    mGame->RemoveAllPlayer();

    const int maxLoadPlayerCount = mGame->GetIsPlayer2Joined() ? 2 : 1;

    int playerNum = 0;
    for (const YAML::Node& node : root["players"]) {
        if (playerNum >= maxLoadPlayerCount) {
            break;
        }

        playerNum++;
        CreatePlayerFromStageNode(node, playerNum);
    }
}

Player* ActorLoadSystem::CreatePlayerFromStageNode(const YAML::Node& node, int playerNum)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];

    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Player> player = std::make_unique<Player>(mGame);

    player->SetPlayerNum(playerNum);
    player->SetCurrentPlanetNum(currentPlanetNum);
    player->SetCurrentPlanet(currentPlanet);

    ApplyPlacementFromStageNode(player.get(), node, currentPlanet, playerNum - 1, 0.0f);
    ApplyRotationFromStageNode(player.get(), node);

    player->ApplyConfig();

    if (node["modelPath"]) {
        player->SetModelPath(node["modelPath"].as<std::string>());
    }

    ApplyScaleFromStageNode(player.get(), node);

    player->Initialize();
    player->SetBaseScale(player->GetScale());

    Player* playerPtr = player.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(playerPtr);
    mGame->AddActor(std::move(player));
    mGame->AddPlayer(playerPtr);

    return playerPtr;
}

bool ActorLoadSystem::CreatePlayerFromCurrentStage(int playerNum)
{
    const std::string& path = mGame->GetCurrentStageYamlPath();

    YAML::Node root = YAML::LoadFile(path);

    if (!root["players"] || !root["players"].IsSequence()) {
        return false;
    }

    const int playerIndex = playerNum - 1;

    if (playerIndex < 0 || playerIndex >= static_cast<int>(root["players"].size())) {
        return false;
    }

    CreatePlayerFromStageNode(root["players"][playerIndex], playerNum);
    return true;
}

void ActorLoadSystem::LoadNPCs(const char* path)
{
    LoadActorSequence<NPC>(
        path, "NPCs", [](Planet* planet) { planet->RemoveAllNPCs(); },
        [this](const YAML::Node& node, int index) { return CreateNPCFromStageNode(node, index); });
}

NPC* ActorLoadSystem::CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<NPC>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "npc.obj", [](Planet* planet, NPC* npc) { planet->AddNPC(npc); },
        [](NPC* npc, const YAML::Node& node) {
            const float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
            npc->SetFacingYaw(facingYaw);

            const float radius = node["radius"] ? node["radius"].as<float>() : 0.75f;
            npc->SetRadius(radius);

            const std::string name = node["name"] ? node["name"].as<std::string>() : "";
            npc->SetName(name);

            if (node["talkTexts"] && node["talkTexts"].IsSequence()) {
                for (const YAML::Node& talkTextNode : node["talkTexts"]) {
                    npc->AddTalkTexts(talkTextNode.as<std::string>());
                }
            }

            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            npc->ApplyConfig(type);
        },
        [](NPC* npc, const YAML::Node&) { npc->SetBaseScale(npc->GetScale()); });
}

void ActorLoadSystem::LoadEnemies(const char* path)
{
    LoadActorSequence<Enemy>(
        path, "enemies", [](Planet* planet) { planet->RemoveAllEnemy(); },
        [this](const YAML::Node& node, int index) { return CreateEnemyFromStageNode(node, index); });
}

Enemy* ActorLoadSystem::CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<Enemy>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "enemy.obj",
        [](Planet* planet, Enemy* enemy) { planet->AddEnemy(enemy); },
        [](Enemy* enemy, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "normal";
            enemy->ApplyConfig(type);
        },
        [](Enemy* enemy, const YAML::Node&) { enemy->SetBaseScale(enemy->GetScale()); });
}

void ActorLoadSystem::LoadPlanets(const char* path)
{
    mGame->GetCurrentStage()->RemoveAllPlanet();

    YAML::Node root = YAML::LoadFile(path);

    if (!root["planets"] || !root["planets"].IsSequence()) {
        return;
    }

    for (const YAML::Node& node : root["planets"]) {
        CreatePlanetFromStageNode(node);
    }
}

Planet* ActorLoadSystem::CreatePlanetFromStageNode(const YAML::Node& node)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    std::unique_ptr<Planet> planet = std::make_unique<Planet>(mGame);

    Stage* currentStage = mGame->GetCurrentStage();

    planet->SetCurrentStage(currentStage);
    planet->ApplyConfig(node);

    planet->Initialize();

    Planet* planetPtr = planet.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(planetPtr);
    mGame->AddActor(std::move(planet));
    currentStage->AddPlanet(planetPtr);

    return planetPtr;
}

void ActorLoadSystem::LoadBoats(const char* path)
{
    LoadActorSequence<Boat>(
        path, "boats", [](Planet* planet) { planet->RemoveAllBoat(); },
        [this](const YAML::Node& node, int index) { return CreateBoatFromStageNode(node, index); });
}

Boat* ActorLoadSystem::CreateBoatFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int startPlanetNum = node["startPlanet"] ? node["startPlanet"].as<int>() : 0;
    int destPlanetNum = node["destPlanet"] ? node["destPlanet"].as<int>() : 0;

    if (startPlanetNum < 0 || startPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    if (destPlanetNum < 0 || destPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[startPlanetNum];
    Planet* destPlanet = planets[destPlanetNum];

    if (!currentPlanet || !destPlanet) {
        return nullptr;
    }

    std::unique_ptr<Boat> boat = std::make_unique<Boat>(mGame);

    boat->SetCurrentPlanet(currentPlanet);
    boat->SetDestPlanet(destPlanet);

    const int arrivalPointIndex = node["arrivalPointIndex"] ? node["arrivalPointIndex"].as<int>() : -1;

    if (arrivalPointIndex >= 0) {
        for (BoatArrivalPoint* point : destPlanet->GetBoatArrivalPoints()) {
            if (point && point->GetStageYamlIndex() == arrivalPointIndex) {
                boat->SetArrivalPoint(point);
                break;
            }
        }
    }

    int destStage = node["destStage"] ? node["destStage"].as<int>() : 0;
    boat->SetDestStage(destStage);

    float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
    boat->SetFacingYaw(facingYaw);

    ApplyPlacementFromStageNode(boat.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(boat.get(), node);

    YAML::Node boatRoot = YAML::LoadFile("../assets/data/actor/boats.yaml");
    for (auto boatNode : boatRoot["boats"]) {
        std::string modelPath = boatNode["modelPath"] ? boatNode["modelPath"].as<std::string>() : "";
        boat->SetModelPath(modelPath);

        float scale = boatNode["scale"] ? boatNode["scale"].as<float>() : 0.25f;
        boat->SetScale(glm::vec3(scale));
    }
    ApplyScaleFromStageNode(boat.get(), node);

    boat->Initialize();

    Boat* boatPtr = boat.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(boatPtr);
    mGame->AddActor(std::move(boat));
    currentPlanet->AddBoat(boatPtr);

    return boatPtr;
}

void ActorLoadSystem::LoadBoatParts(const char* path)
{
    LoadActorSequence<BoatParts>(
        path, "boatParts", [](Planet* planet) { planet->RemoveAllBoatParts(); },
        [this](const YAML::Node& node, int index) { return CreateBoatPartsFromStageNode(node, index); });
}

BoatParts* ActorLoadSystem::CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<BoatParts>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, BoatParts* boatParts) {
            planet->AddBoatParts(boatParts);
            planet->Initialize();
        },
        [](BoatParts* boatParts, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            boatParts->ApplyConfig(type);
        });
}

void ActorLoadSystem::LoadKeys(const char* path)
{
    LoadActorSequence<Key>(
        path, "keys", [](Planet* planet) { planet->RemoveKey(); },
        [this](const YAML::Node& node, int index) { return CreateKeyFromStageNode(node, index); });
}

Key* ActorLoadSystem::CreateKeyFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<Key>(
        node, stageYamlIndex, 0.0f, glm::vec3(0.25f), "key.obj", [](Planet* planet, Key* key) { planet->SetKey(key); },
        [](Key* key, const YAML::Node&) { key->ApplyConfig(); });
}

void ActorLoadSystem::LoadCrystals(const char* path)
{
    LoadActorSequence<Crystal>(
        path, "crystals", [](Planet* planet) { planet->RemoveAllCrystals(); },
        [this](const YAML::Node& node, int index) { return CreateCrystalFromStageNode(node, index); });
}

Crystal* ActorLoadSystem::CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<Crystal>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, Crystal* crystal) { planet->AddCrystal(crystal); },
        [](Crystal* crystal, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            crystal->ApplyConfig(type);
        });
}

void ActorLoadSystem::LoadStar(const char* path)
{
    LoadActorSequence<Star>(
        path, "star", [](Planet* planet) { planet->RemoveStar(); },
        [this](const YAML::Node& node, int index) { return CreateStarFromStageNode(node, index); });
}

Star* ActorLoadSystem::CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<Star>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.0f), "star.obj",
        [](Planet* planet, Star* star) { planet->SetStar(star); },
        [](Star* star, const YAML::Node&) { star->ApplyConfig(); },
        [](Star* star, const YAML::Node& node) {
            if (node["isActive"]) {
                star->SetIsActive(node["isActive"].as<bool>());
            }
        });
}

void ActorLoadSystem::LoadPlatforms(const char* path)
{
    LoadActorSequence<Platform>(
        path, "platforms", [](Planet* planet) { planet->RemoveAllPlatforms(); },
        [this](const YAML::Node& node, int index) { return CreatePlatformFromStageNode(node, index); });
}

Platform* ActorLoadSystem::CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<Platform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, Platform* platform) { planet->AddPlatform(platform); });
}

void ActorLoadSystem::LoadMovingPlatforms(const char* path)
{
    LoadActorSequence<MovingPlatform>(
        path, "movingPlatforms", [](Planet* planet) { planet->RemoveAllMovingPlatforms(); },
        [this](const YAML::Node& node, int index) { return CreateMovingPlatformFromStageNode(node, index); });
}

MovingPlatform* ActorLoadSystem::CreateMovingPlatformFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<MovingPlatform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, MovingPlatform* platform) { planet->AddMovingPlatform(platform); },
        [](MovingPlatform* platform, const YAML::Node& node) {
            glm::vec3 moveOffset(4.0f, 0.0f, 0.0f);

            if (node["moveOffset"] && node["moveOffset"].IsSequence() && node["moveOffset"].size() >= 3) {
                moveOffset.x = node["moveOffset"][0] ? node["moveOffset"][0].as<float>() : moveOffset.x;
                moveOffset.y = node["moveOffset"][1] ? node["moveOffset"][1].as<float>() : moveOffset.y;
                moveOffset.z = node["moveOffset"][2] ? node["moveOffset"][2].as<float>() : moveOffset.z;
            }

            platform->SetMoveOffset(moveOffset);

            const float moveDuration = node["moveDuration"] ? node["moveDuration"].as<float>() : 3.0f;
            platform->SetMoveDuration(moveDuration);

            if (platform->GetCurrentPlanet()) {
                platform->SetBaseLocalPos(platform->GetPos() - platform->GetCurrentPlanet()->GetPos());
            }
        });
}

void ActorLoadSystem::LoadBoatArrivalPoints(const char* path)
{
    LoadActorSequence<BoatArrivalPoint>(
        path, "boatArrivalPoints", [](Planet* planet) { planet->RemoveAllBoatArrivalPoints(); },
        [this](const YAML::Node& node, int index) { return CreateBoatArrivalPointFromStageNode(node, index); });
}

BoatArrivalPoint* ActorLoadSystem::CreateBoatArrivalPointFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<BoatArrivalPoint>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.4f), "platform.obj",
        [](Planet* planet, BoatArrivalPoint* point) { planet->AddBoatArrivalPoint(point); },
        [](BoatArrivalPoint* point, const YAML::Node&) { point->ApplyConfig(); });
}

void ActorLoadSystem::LoadFallRespawnPoints(const char* path)
{
    LoadActorSequence<FallRespawnPoint>(
        path, "fallRespawnPoints", [](Planet* planet) { planet->RemoveAllFallRespawnPoints(); },
        [this](const YAML::Node& node, int index) { return CreateFallRespawnPointFromStageNode(node, index); });
}

FallRespawnPoint* ActorLoadSystem::CreateFallRespawnPointFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return CreatePlacedActorFromStageNode<FallRespawnPoint>(
        node, stageYamlIndex, 0.0f, glm::vec3(4.0f, 1.0f, 4.0f), "platform.obj",
        [](Planet* planet, FallRespawnPoint* point) { planet->AddFallRespawnPoint(point); },
        [](FallRespawnPoint* point, const YAML::Node&) { point->ApplyConfig(); },
        [](FallRespawnPoint* point, const YAML::Node& node) {
            if (node["damage"]) {
                point->SetDamage(node["damage"].as<float>());
            }
        });
}

glm::vec3 ActorLoadSystem::CalculatePos(YAML::Node node, Planet* currentPlanet)
{
    if (node["pos"]) {
        float posX = node["pos"][0].as<float>();
        float posY = node["pos"][1].as<float>();
        float posZ = node["pos"][2].as<float>();
        glm::vec3 pos = currentPlanet->GetPos() + glm::vec3(posX, posY, posZ);
        return pos;
    }

    float theta = node["theta"] ? node["theta"].as<float>() : 0.0f;
    float phi = node["phi"] ? node["phi"].as<float>() : 0.0f;
    float height = node["height"] ? node["height"].as<float>() : 0.0f;
    glm::vec3 dir(std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta));
    float len = glm::length(dir);
    if (len < 1e-6f)
        dir = glm::vec3(1.0f, 0.0f, 0.0f);
    else
        dir /= len;

    glm::vec3 pos = currentPlanet->GetPos() + (currentPlanet->GetRadius() + height) * dir;
    return pos;
}

void ActorLoadSystem::ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet,
                                                  int stageYamlIndex, float defaultHeight)
{
    if (!actor || !currentPlanet) {
        return;
    }

    actor->SetStageYamlIndex(stageYamlIndex);

    const float theta = node["theta"] ? node["theta"].as<float>() : 0.0f;
    const float phi = node["phi"] ? node["phi"].as<float>() : 0.0f;
    const float height = node["height"] ? node["height"].as<float>() : defaultHeight;

    actor->SetSphericalPlacement(theta, phi, height);

    const bool hasPos = node["pos"] && node["pos"].IsSequence() && node["pos"].size() >= 3;

    if (hasPos) {
        actor->SetPos(CalculatePos(node, currentPlanet));
    } else {
        actor->SetPos(currentPlanet->CalculateSurfacePos(theta, phi, height));
    }
}

void ActorLoadSystem::ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node)
{
    if (!actor) {
        return;
    }

    glm::vec3 editorRotation(0.0f);

    if (node["facingYaw"]) {
        editorRotation.y = node["facingYaw"].as<float>();
    }

    if (node["rotation"] && node["rotation"].IsSequence() && node["rotation"].size() >= 3) {
        editorRotation.x = node["rotation"][0].as<float>();
        editorRotation.y = node["rotation"][1].as<float>();
        editorRotation.z = node["rotation"][2].as<float>();
    }

    actor->SetEditorRotation(editorRotation);
    actor->SetFacingYaw(editorRotation.y);

    if (node["upVec"] && node["upVec"].IsSequence() && node["upVec"].size() >= 3) {
        glm::vec3 upVec(node["upVec"][0].as<float>(), node["upVec"][1].as<float>(), node["upVec"][2].as<float>());

        if (glm::length(upVec) > 1e-6f) {
            actor->SetUpVec(glm::normalize(upVec));
        }
    }
}

void ActorLoadSystem::ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node)
{
    if (!actor) {
        return;
    }

    if (!node["scale"] || !node["scale"].IsSequence() || node["scale"].size() < 3) {
        return;
    }

    const glm::vec3 currentScale = actor->GetScale();

    const float scaleX = node["scale"][0] ? node["scale"][0].as<float>() : currentScale.x;
    const float scaleY = node["scale"][1] ? node["scale"][1].as<float>() : currentScale.y;
    const float scaleZ = node["scale"][2] ? node["scale"][2].as<float>() : currentScale.z;

    actor->SetScale(glm::vec3(scaleX, scaleY, scaleZ));
}