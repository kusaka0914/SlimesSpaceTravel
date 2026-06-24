#include "ActorLoadSystem.h"
#include "Game.h"
#include "Stage.h"
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
#include "component/DestructibleComponent.h"
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
    LoadBoats(path.c_str());
    LoadBoatParts(path.c_str());
    LoadKeys(path.c_str());
    LoadCrystals(path.c_str());
    LoadStar(path.c_str());
    LoadNPCs(path.c_str());
    LoadPlatforms(path.c_str());
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
    std::unique_ptr<Player> player = std::make_unique<Player>(mGame);

    player->SetPlayerNum(playerNum);

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
    player->SetCurrentPlanetNum(currentPlanetNum);

    Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
    player->SetCurrentPlanet(currentPlanet);

    glm::vec3 pos = CalculatePos(node, currentPlanet);
    player->SetPos(pos);

    YAML::Node playerRoot = YAML::LoadFile("../assets/data/actor/players.yaml");
    for (auto playerNode : playerRoot["players"]) {
        float hp = playerNode["hp"] ? playerNode["hp"].as<float>() : 0.0f;
        player->SetHp(hp);
        player->SetMaxHp(hp);

        float scale = playerNode["scale"] ? playerNode["scale"].as<float>() : 0.25f;
        player->SetScale(glm::vec3(scale));

        float attackSpeed = playerNode["attackSpeed"] ? playerNode["attackSpeed"].as<float>() : 0.0f;
        player->SetAttackSpeed(attackSpeed);

        float attack = playerNode["attack"] ? playerNode["attack"].as<float>() : 0.0f;
        player->SetAttack(attack);

        float moveSpeed = playerNode["moveSpeed"] ? playerNode["moveSpeed"].as<float>() : 0.0f;
        player->SetMoveSpeed(moveSpeed);

        float dodgeDuration = playerNode["dodgeDuration"] ? playerNode["dodgeDuration"].as<float>() : 0.0f;
        player->SetDodgeDuration(dodgeDuration);

        float dodgeCooldownTime = playerNode["dodgeCooldownTime"] ? playerNode["dodgeCooldownTime"].as<float>() : 0.0f;
        player->SetDodgeCooldownTime(dodgeCooldownTime);

        float dodgeDistance = playerNode["dodgeDistance"] ? playerNode["dodgeDistance"].as<float>() : 0.0f;
        player->SetDodgeDistance(dodgeDistance);

        float normalAttackRange = playerNode["normalAttackRange"] ? playerNode["normalAttackRange"].as<float>() : 0.0f;
        player->SetNormalAttackRange(normalAttackRange);

        float normalAttackAngle = playerNode["normalAttackAngle"] ? playerNode["normalAttackAngle"].as<float>() : 0.0f;
        player->SetNormalAttackAngle(normalAttackAngle);

        float normalAttack = playerNode["normalAttack"] ? playerNode["normalAttack"].as<float>() : 0.0f;
        player->SetNormalAttack(normalAttack);

        float wideAttackRange = playerNode["wideAttackRange"] ? playerNode["wideAttackRange"].as<float>() : 0.0f;
        player->SetWideAttackRange(wideAttackRange);

        float wideAttackAngle = playerNode["wideAttackAngle"] ? playerNode["wideAttackAngle"].as<float>() : 0.0f;
        player->SetWideAttackAngle(wideAttackAngle);

        float wideAttack = playerNode["wideAttack"] ? playerNode["wideAttack"].as<float>() : 0.0f;
        player->SetWideAttack(wideAttack);

        float strongAttackRange = playerNode["strongAttackRange"] ? playerNode["strongAttackRange"].as<float>() : 0.0f;
        player->SetStrongAttackRange(strongAttackRange);

        float strongAttack = playerNode["strongAttack"] ? playerNode["strongAttack"].as<float>() : 0.0f;
        player->SetStrongAttack(strongAttack);

        float strongAttackSpeed = playerNode["strongAttackSpeed"] ? playerNode["strongAttackSpeed"].as<float>() : 0.0f;
        player->SetStrongAttackSpeed(strongAttackSpeed);

        float specialAttackCooldown =
            playerNode["specialAttackCooldown"] ? playerNode["specialAttackCooldown"].as<float>() : 0.0f;
        player->SetSpecialAttackCooldown(specialAttackCooldown);

        float defaultInvincibleTimer =
            playerNode["defaultInvincibleTimer"] ? playerNode["defaultInvincibleTimer"].as<float>() : 0.0f;
        player->SetDefaultInvincibleTimer(defaultInvincibleTimer);

        float defaultDamageTimer =
            playerNode["defaultDamageTimer"] ? playerNode["defaultDamageTimer"].as<float>() : 0.0f;
        player->SetDefaultDamageTimer(defaultDamageTimer);

        float defaultAttackMotionTimer =
            playerNode["defaultAttackMotionTimer"] ? playerNode["defaultAttackMotionTimer"].as<float>() : 0.0f;
        player->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);

        float attackCooldown = playerNode["attackCooldown"] ? playerNode["attackCooldown"].as<float>() : 0.0f;
        player->SetAttackCooldown(attackCooldown);

        float lastAttackCooldown =
            playerNode["lastAttackCooldown"] ? playerNode["lastAttackCooldown"].as<float>() : 0.0f;
        player->SetLastAttackCooldown(lastAttackCooldown);

        float defaultAttackPressTimer =
            playerNode["defaultAttackPressTimer"] ? playerNode["defaultAttackPressTimer"].as<float>() : 0.0f;
        player->SetDefaultAttackPressTimer(defaultAttackPressTimer);

        float chargeMoveSpeed = playerNode["chargeMoveSpeed"] ? playerNode["chargeMoveSpeed"].as<float>() : 0.0f;
        player->SetChargeMoveSpeed(chargeMoveSpeed);

        float defaultStrongAttackTimer =
            playerNode["defaultStrongAttackTimer"] ? playerNode["defaultStrongAttackTimer"].as<float>() : 0.0f;
        player->SetDefaultStrongAttackTimer(defaultStrongAttackTimer);

        float knockBackSpeed = playerNode["knockBackSpeed"] ? playerNode["knockBackSpeed"].as<float>() : 0.0f;
        player->SetKnockBackSpeed(knockBackSpeed);

        std::string modelPath = node["modelPath"] ? node["modelPath"].as<std::string>() : "player.obj";
        player->SetModelPath(modelPath);
    }

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
    YAML::Node root = YAML::LoadFile(path);

    if (!root["NPCs"] || !root["NPCs"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllNPCs();
        }
    }

    YAML::Node npcsNode = root["NPCs"];

    for (std::size_t i = 0; i < npcsNode.size(); ++i) {
        CreateNPCFromStageNode(npcsNode[i], static_cast<int>(i));
    }
}

NPC* ActorLoadSystem::CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<NPC> npc = std::make_unique<NPC>(mGame);

    npc->SetCurrentPlanet(currentPlanet);

    float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
    npc->SetFacingYaw(facingYaw);

    float radius = node["radius"] ? node["radius"].as<float>() : 0.75f;
    npc->SetRadius(radius);

    std::string name = node["name"] ? node["name"].as<std::string>() : "";
    npc->SetName(name);

    if (node["talkTexts"]) {
        for (auto talkTextNode : node["talkTexts"]) {
            std::string talkText = talkTextNode.as<std::string>();
            npc->AddTalkTexts(talkText);
        }
    }

    ApplyPlacementFromStageNode(npc.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(npc.get(), node);

    std::string type = node["type"] ? node["type"].as<std::string>() : "";

    YAML::Node npcRoot = YAML::LoadFile("../assets/data/actor/npcs.yaml");
    for (auto npcNode : npcRoot["npcs"]) {
        if (type != npcNode["type"].as<std::string>()) {
            continue;
        }

        std::string modelPath = npcNode["modelPath"] ? npcNode["modelPath"].as<std::string>() : "npc.obj";
        npc->SetModelPath(modelPath);

        float scale = npcNode["scale"] ? npcNode["scale"].as<float>() : 0.25f;
        npc->SetScale(glm::vec3(scale));
    }

    npc->SetBaseScale(npc->GetScale());

    NPC* npcPtr = npc.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(npcPtr);
    mGame->AddActor(std::move(npc));
    currentPlanet->AddNPC(npcPtr);

    return npcPtr;
}

void ActorLoadSystem::LoadEnemies(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["enemies"] || !root["enemies"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllEnemy();
        }
    }

    YAML::Node enemiesNode = root["enemies"];

    for (std::size_t i = 0; i < enemiesNode.size(); ++i) {
        CreateEnemyFromStageNode(enemiesNode[i], static_cast<int>(i));
    }
}

Enemy* ActorLoadSystem::CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Enemy> enemy = std::make_unique<Enemy>(mGame);

    enemy->SetCurrentPlanet(currentPlanet);

    ApplyPlacementFromStageNode(enemy.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(enemy.get(), node);

    const std::string type = node["type"] ? node["type"].as<std::string>() : "normal";

    if (type == "boss") {
        enemy->SetIsBoss(true);
    } else {
        enemy->SetIsBoss(false);
    }

    ApplyEnemyConfig(enemy.get(), type);

    enemy->SetBaseScale(enemy->GetScale());

    Enemy* enemyPtr = enemy.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(enemyPtr);
    mGame->AddActor(std::move(enemy));
    currentPlanet->AddEnemy(enemyPtr);

    return enemyPtr;
}

void ActorLoadSystem::ApplyEnemyConfig(Enemy* enemy, const std::string& type)
{
    if (!enemy) {
        return;
    }

    YAML::Node enemyRoot = YAML::LoadFile("../assets/data/actor/enemies.yaml");

    for (auto enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = enemyNode["type"] ? enemyNode["type"].as<std::string>() : "";

        if (enemyType == "common") {
            float knockBackSpeed = enemyNode["knockBackSpeed"] ? enemyNode["knockBackSpeed"].as<float>() : 0.0f;
            enemy->SetKnockBackSpeed(knockBackSpeed);

            float defaultLaunchedTimer =
                enemyNode["defaultLaunchedTimer"] ? enemyNode["defaultLaunchedTimer"].as<float>() : 0.0f;
            enemy->SetDefaultLaunchedTimer(defaultLaunchedTimer);

            float detectionRange = enemyNode["detectionRange"] ? enemyNode["detectionRange"].as<float>() : 0.0f;
            enemy->SetDetectionRange(detectionRange);

            continue;
        }

        if (type != enemyType) {
            continue;
        }

        float hp = enemyNode["hp"] ? enemyNode["hp"].as<float>() : 80.0f;
        enemy->SetHp(hp);
        enemy->SetMaxHp(hp);

        float scale = enemyNode["scale"] ? enemyNode["scale"].as<float>() : 0.25f;
        enemy->SetScale(glm::vec3(scale));

        float speed = enemyNode["speed"] ? enemyNode["speed"].as<float>() : 1.0f;
        enemy->SetMoveSpeed(speed);

        float attack = enemyNode["attack"] ? enemyNode["attack"].as<float>() : 5.0f;
        enemy->SetAttack(attack);

        float radius = enemyNode["radius"] ? enemyNode["radius"].as<float>() : 0.75f;
        enemy->SetRadius(radius);

        int breakCountMax = enemyNode["breakCountMax"] ? enemyNode["breakCountMax"].as<int>() : 1;
        enemy->SetBreakCountMax(breakCountMax);
        enemy->SetBreakCount(breakCountMax);

        std::string modelPath = enemyNode["modelPath"] ? enemyNode["modelPath"].as<std::string>() : "";
        enemy->SetModelPath(modelPath);

        float defaultStandByAttackTimer =
            enemyNode["defaultStandByAttackTimer"] ? enemyNode["defaultStandByAttackTimer"].as<float>() : 0.0f;
        enemy->SetDefaultStandByAttackTimer(defaultStandByAttackTimer);

        float defaultAttackMotionTimer =
            enemyNode["defaultAttackMotionTimer"] ? enemyNode["defaultAttackMotionTimer"].as<float>() : 0.0f;
        enemy->SetDefaultAttackMotionTimer(defaultAttackMotionTimer);

        float attackSpeed = enemyNode["attackSpeed"] ? enemyNode["attackSpeed"].as<float>() : 0.0f;
        enemy->SetAttackSpeed(attackSpeed);
    }
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

    if (node["center"]) {
        float x = node["center"][0] ? node["center"][0].as<float>() : 0.0f;
        float y = node["center"][1] ? node["center"][1].as<float>() : 0.0f;
        float z = node["center"][2] ? node["center"][2].as<float>() : 0.0f;
        planet->SetPos(glm::vec3(x, y, z));
    } else {
        planet->SetPos(glm::vec3(0.0f));
    }

    if (node["scale"]) {
        float scaleX = node["scale"][0] ? node["scale"][0].as<float>() : 1.0f;
        float scaleY = node["scale"][1] ? node["scale"][1].as<float>() : 1.0f;
        float scaleZ = node["scale"][2] ? node["scale"][2].as<float>() : 1.0f;

        glm::vec3 scale(scaleX, scaleY, scaleZ);
        planet->SetScale(scale);
        planet->SetRadius(scaleX);
    } else {
        planet->SetScale(glm::vec3(1.0f));
        planet->SetRadius(1.0f);
    }

    if (node["color"]) {
        float r = node["color"][0] ? node["color"][0].as<float>() : 1.0f;
        float g = node["color"][1] ? node["color"][1].as<float>() : 1.0f;
        float b = node["color"][2] ? node["color"][2].as<float>() : 1.0f;
        float a = node["color"][3] ? node["color"][3].as<float>() : 1.0f;
        planet->SetColor(glm::vec4(r, g, b, a));
    } else {
        planet->SetColor(glm::vec4(1.0f));
    }

    std::string modelPath = node["model"] ? node["model"].as<std::string>() : "planet.obj";
    planet->SetModelPath(modelPath);

    std::string shape = node["shape"] ? node["shape"].as<std::string>() : "Sphere";
    planet->SetPlanetShape(shape);

    int stageNum = node["stageNum"] ? node["stageNum"].as<int>() : 0;
    planet->SetStageNum(stageNum);

    std::string rocketSpawnCondition =
        node["rocketSpawnCondition"] ? node["rocketSpawnCondition"].as<std::string>() : "";
    planet->SetRocketSpawnCondition(rocketSpawnCondition);

    Stage* currentStage = mGame->GetCurrentStage();
    planet->SetCurrentStage(currentStage);

    planet->Initialize();

    Planet* planetPtr = planet.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(planetPtr);
    mGame->AddActor(std::move(planet));
    currentStage->AddPlanet(planetPtr);

    return planetPtr;
}

void ActorLoadSystem::LoadBoats(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["boats"] || !root["boats"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllBoat();
        }
    }

    YAML::Node boatsNode = root["boats"];

    for (std::size_t i = 0; i < boatsNode.size(); ++i) {
        CreateBoatFromStageNode(boatsNode[i], static_cast<int>(i));
    }
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

    boat->Initialize();

    Boat* boatPtr = boat.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(boatPtr);
    mGame->AddActor(std::move(boat));
    currentPlanet->AddBoat(boatPtr);

    return boatPtr;
}

void ActorLoadSystem::LoadBoatParts(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["boatParts"] || !root["boatParts"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllBoatParts();
        }
    }

    YAML::Node boatPartsNode = root["boatParts"];

    for (std::size_t i = 0; i < boatPartsNode.size(); ++i) {
        CreateBoatPartsFromStageNode(boatPartsNode[i], static_cast<int>(i));
    }
}

BoatParts* ActorLoadSystem::CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<BoatParts> boatParts = std::make_unique<BoatParts>(mGame);

    boatParts->SetCurrentPlanet(currentPlanet);

    ApplyPlacementFromStageNode(boatParts.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(boatParts.get(), node);

    std::string type = node["type"] ? node["type"].as<std::string>() : "";

    YAML::Node boatPartsRoot = YAML::LoadFile("../assets/data/actor/boatparts.yaml");
    for (auto boatPartsInfoNode : boatPartsRoot["boatParts"]) {
        if (boatPartsInfoNode["type"].as<std::string>() == "common") {
            float scale = boatPartsInfoNode["scale"] ? boatPartsInfoNode["scale"].as<float>() : 0.25f;
            boatParts->SetScale(glm::vec3(scale));
            continue;
        }

        if (type != boatPartsInfoNode["type"].as<std::string>()) {
            continue;
        }

        std::string modelPath = boatPartsInfoNode["modelPath"] ? boatPartsInfoNode["modelPath"].as<std::string>() : "";
        boatParts->SetModelPath(modelPath);
    }

    BoatParts* boatPartsPtr = boatParts.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(boatPartsPtr);
    mGame->AddActor(std::move(boatParts));
    currentPlanet->AddBoatParts(boatPartsPtr);

    currentPlanet->Initialize();

    return boatPartsPtr;
}

void ActorLoadSystem::LoadKeys(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["keys"] || !root["keys"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveKey();
        }
    }

    YAML::Node keysNode = root["keys"];

    for (std::size_t i = 0; i < keysNode.size(); ++i) {
        YAML::Node node = keysNode[i];

        std::unique_ptr<Key> key = std::make_unique<Key>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        key->SetCurrentPlanet(currentPlanet);

        float theta = node["theta"] ? node["theta"].as<float>() : 0.0f;
        float phi = node["phi"] ? node["phi"].as<float>() : 0.0f;
        float height = node["height"] ? node["height"].as<float>() : 0.0f;

        key->SetSphericalPlacement(theta, phi, height);
        key->SetStageYamlIndex(static_cast<int>(i));

        glm::vec3 pos = currentPlanet->CalculateSurfacePos(theta, phi, height);
        key->SetPos(pos);

        YAML::Node keyRoot = YAML::LoadFile("../assets/data/actor/keys.yaml");
        for (auto keyNode : keyRoot["keys"]) {
            std::string modelPath = keyNode["modelPath"] ? keyNode["modelPath"].as<std::string>() : "key.obj";
            key->SetModelPath(modelPath);

            float scale = keyNode["scale"] ? keyNode["scale"].as<float>() : 0.25f;
            key->SetScale(glm::vec3(scale));
        }

        Key* keyPtr = key.get();
        mGame->GetMeshLoadSystem()->SetActorMesh(keyPtr);
        mGame->AddActor(std::move(key));
        currentPlanet->SetKey(keyPtr);
    }
}

void ActorLoadSystem::LoadCrystals(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["crystals"] || !root["crystals"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllCrystals();
        }
    }

    YAML::Node crystalsNode = root["crystals"];

    for (std::size_t i = 0; i < crystalsNode.size(); ++i) {
        CreateCrystalFromStageNode(crystalsNode[i], static_cast<int>(i));
    }
}

Crystal* ActorLoadSystem::CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Crystal> crystal = std::make_unique<Crystal>(mGame);

    crystal->SetCurrentPlanet(currentPlanet);

    std::string type = node["type"] ? node["type"].as<std::string>() : "";

    YAML::Node crystalRoot = YAML::LoadFile("../assets/data/actor/crystals.yaml");
    for (auto crystalNode : crystalRoot["crystals"]) {
        if (crystalNode["type"].as<std::string>() == "common") {
            std::string modelPath = crystalNode["modelPath"] ? crystalNode["modelPath"].as<std::string>() : "";
            crystal->SetModelPath(modelPath);
            continue;
        }

        if (type != crystalNode["type"].as<std::string>()) {
            continue;
        }

        float hp = crystalNode["hp"] ? crystalNode["hp"].as<float>() : 80.0f;
        crystal->GetDestructibleComponent()->SetDestroyHp(hp);

        float scale = crystalNode["scale"] ? crystalNode["scale"].as<float>() : 0.25f;
        crystal->SetScale(glm::vec3(scale));

        float radius = crystalNode["radius"] ? crystalNode["radius"].as<float>() : 1.0f;
        crystal->SetRadius(radius);
    }

    ApplyPlacementFromStageNode(crystal.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(crystal.get(), node);

    Crystal* crystalPtr = crystal.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(crystalPtr);
    mGame->AddActor(std::move(crystal));
    currentPlanet->AddCrystal(crystalPtr);

    return crystalPtr;
}

void ActorLoadSystem::LoadStar(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["star"] || !root["star"].IsSequence()) {
        return;
    }

    YAML::Node starNode = root["star"];

    for (std::size_t i = 0; i < starNode.size(); ++i) {
        CreateStarFromStageNode(starNode[i], static_cast<int>(i));
    }
}

Star* ActorLoadSystem::CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Star> star = std::make_unique<Star>(mGame);

    star->SetCurrentPlanet(currentPlanet);

    YAML::Node starRoot = YAML::LoadFile("../assets/data/actor/stars.yaml");
    for (auto starNode : starRoot["stars"]) {
        std::string modelPath = starNode["modelPath"] ? starNode["modelPath"].as<std::string>() : "star.obj";
        star->SetModelPath(modelPath);

        float scale = starNode["scale"] ? starNode["scale"].as<float>() : 0.0f;
        star->SetScale(glm::vec3(scale));
    }

    ApplyPlacementFromStageNode(star.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(star.get(), node);

    if (node["isActive"]) {
        star->SetIsActive(node["isActive"].as<bool>());
    }

    Star* starPtr = star.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(starPtr);
    mGame->AddActor(std::move(star));
    currentPlanet->SetStar(starPtr);

    return starPtr;
}

void ActorLoadSystem::LoadPlatforms(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    if (!root["platforms"] || !root["platforms"].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet) {
            planet->RemoveAllPlatforms();
        }
    }

    YAML::Node platformsNode = root["platforms"];

    for (std::size_t i = 0; i < platformsNode.size(); ++i) {
        CreatePlatformFromStageNode(platformsNode[i], static_cast<int>(i));
    }
}

Platform* ActorLoadSystem::CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    int currentPlanetNum = 0;

    if (node["currentPlanetNum"]) {
        currentPlanetNum = node["currentPlanetNum"].as<int>();
    }

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Platform> platform = std::make_unique<Platform>(mGame);

    platform->SetCurrentPlanet(currentPlanet);

    ApplyPlacementFromStageNode(platform.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    ApplyRotationFromStageNode(platform.get(), node);

    if (node["scale"]) {
        const float scaleX = node["scale"][0] ? node["scale"][0].as<float>() : 3.0f;
        const float scaleY = node["scale"][1] ? node["scale"][1].as<float>() : 0.5f;
        const float scaleZ = node["scale"][2] ? node["scale"][2].as<float>() : 3.0f;

        platform->SetScale(glm::vec3(scaleX, scaleY, scaleZ));
    } else {
        platform->SetScale(glm::vec3(3.0f, 0.5f, 3.0f));
    }

    std::string modelPath = node["modelPath"] ? node["modelPath"].as<std::string>() : "platform.obj";
    platform->SetModelPath(modelPath);

    platform->Initialize();

    Platform* platformPtr = platform.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(platformPtr);

    mGame->AddActor(std::move(platform));
    currentPlanet->AddPlatform(platformPtr);

    return platformPtr;
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