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
        // std::cerr << "ActorLoadSystem: missing or invalid 'players' sequence" << std::endl;
        return;
    }

    mGame->RemoveAllPlayer();
    int playerNum = 0;
    for (const YAML::Node& node : root["players"]) {
        std::unique_ptr<Player> player = std::make_unique<Player>(mGame);
        playerNum++;
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

            float dodgeCooldownTime =
                playerNode["dodgeCooldownTime"] ? playerNode["dodgeCooldownTime"].as<float>() : 0.0f;
            player->SetDodgeCooldownTime(dodgeCooldownTime);

            float dodgeDistance = playerNode["dodgeDistance"] ? playerNode["dodgeDistance"].as<float>() : 0.0f;
            player->SetDodgeDistance(dodgeDistance);

            float normalAttackRange =
                playerNode["normalAttackRange"] ? playerNode["normalAttackRange"].as<float>() : 0.0f;
            player->SetNormalAttackRange(normalAttackRange);

            float normalAttackAngle =
                playerNode["normalAttackAngle"] ? playerNode["normalAttackAngle"].as<float>() : 0.0f;
            player->SetNormalAttackAngle(normalAttackAngle);

            float normalAttack = playerNode["normalAttack"] ? playerNode["normalAttack"].as<float>() : 0.0f;
            player->SetNormalAttack(normalAttack);

            float wideAttackRange = playerNode["wideAttackRange"] ? playerNode["wideAttackRange"].as<float>() : 0.0f;
            player->SetWideAttackRange(wideAttackRange);

            float wideAttackAngle = playerNode["wideAttackAngle"] ? playerNode["wideAttackAngle"].as<float>() : 0.0f;
            player->SetWideAttackAngle(wideAttackAngle);

            float wideAttack = playerNode["wideAttack"] ? playerNode["wideAttack"].as<float>() : 0.0f;
            player->SetWideAttack(wideAttack);

            float strongAttackRange =
                playerNode["strongAttackRange"] ? playerNode["strongAttackRange"].as<float>() : 0.0f;
            player->SetStrongAttackRange(strongAttackRange);

            float strongAttack = playerNode["strongAttack"] ? playerNode["strongAttack"].as<float>() : 0.0f;
            player->SetStrongAttack(strongAttack);

            float strongAttackSpeed =
                playerNode["strongAttackSpeed"] ? playerNode["strongAttackSpeed"].as<float>() : 0.0f;
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
        Player* player_ptr = player.get();
        mGame->AddActor(std::move(player));
        mGame->AddPlayer(player_ptr);
    }
}

void ActorLoadSystem::LoadNPCs(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);
    if (!root["NPCs"] || !root["NPCs"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'NPCs' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["NPCs"]) {
        std::unique_ptr<NPC> npc = std::make_unique<NPC>(mGame);

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

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        npc->SetCurrentPlanet(currentPlanet);

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        npc->SetPos(pos);

        std::string type = node["type"] ? node["type"].as<std::string>() : "";
        YAML::Node npcRoot = YAML::LoadFile("../assets/data/actor/npcs.yaml");
        for (auto npcNode : npcRoot["npcs"]) {
            if (type != npcNode["type"].as<std::string>()) {
                continue;
            }

            std::string modelPath = npcNode["modelPath"] ? npcNode["modelPath"].as<std::string>() : "npc.obj";
            npc->SetModelPath(modelPath);

            float scale = npcNode["scale"] ? npcNode["scale"].as<float>() : 0.0f;
            npc->SetScale(glm::vec3(scale));
        }

        NPC* npc_ptr = npc.get();
        mGame->AddActor(std::move(npc));
        currentPlanet->AddNPC(npc_ptr);
    }
}

void ActorLoadSystem::LoadEnemies(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["enemies"] || !root["enemies"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'enemies' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["enemies"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveAllEnemy();
    }

    for (const YAML::Node& node : root["enemies"]) {
        std::unique_ptr<Enemy> enemy = std::make_unique<Enemy>(mGame);

        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        enemy->SetCurrentPlanet(currentPlanet);

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        enemy->SetPos(pos);

        std::string type = node["type"] ? node["type"].as<std::string>() : "";
        if (type == "boss")
            enemy->SetIsBoss(true);
        YAML::Node enemyRoot = YAML::LoadFile("../assets/data/actor/enemies.yaml");
        for (auto enemyNode : enemyRoot["enemies"]) {
            if (enemyNode["type"].as<std::string>() == "common") {
                float knockBackSpeed = enemyNode["knockBackSpeed"] ? enemyNode["knockBackSpeed"].as<float>() : 0.0f;
                enemy->SetKnockBackSpeed(knockBackSpeed);

                float defaultLaunchedTimer =
                    enemyNode["defaultLaunchedTimer"] ? enemyNode["defaultLaunchedTimer"].as<float>() : 0.0f;
                enemy->SetDefaultLaunchedTimer(defaultLaunchedTimer);

                float detectionRange = enemyNode["detectionRange"] ? enemyNode["detectionRange"].as<float>() : 0.0f;
                enemy->SetDetectionRange(detectionRange);
                continue;
            }

            if (type != enemyNode["type"].as<std::string>())
                continue;

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

        Enemy* enemy_ptr = enemy.get();
        mGame->AddActor(std::move(enemy));
        currentPlanet->AddEnemy(enemy_ptr);
    }
}

void ActorLoadSystem::LoadPlanets(const char* path)
{
    mGame->GetCurrentStage()->RemoveAllPlanet();
    YAML::Node root = YAML::LoadFile(path);

    if (!root["planets"] || !root["planets"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'planets' sequence" << std::endl;
        return;
    }

    for (const YAML::Node& node : root["planets"]) {

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
            glm::vec3 scale = glm::vec3(scaleX, scaleY, scaleZ);
            planet->SetScale(scale);
            planet->SetRadius(scaleX);
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

        std::string modelPath = node["model"] ? node["model"].as<std::string>() : "";
        planet->SetModelPath(modelPath);

        std::string shape = node["shape"] ? node["shape"].as<std::string>() : "Normal";
        planet->SetPlanetShape(shape);

        int stageNum = node["stageNum"] ? node["stageNum"].as<int>() : 0;
        planet->SetStageNum(stageNum);

        std::string rocketSpawnCondition =
            node["rocketSpawnCondition"] ? node["rocketSpawnCondition"].as<std::string>() : "";
        planet->SetRocketSpawnCondition(rocketSpawnCondition);

        Stage* currentStage = mGame->GetCurrentStage();
        planet->SetCurrentStage(currentStage);

        planet->Initialize();
        Planet* planet_ptr = planet.get();
        mGame->AddActor(std::move(planet));
        currentStage->AddPlanet(planet_ptr);
    }
}

void ActorLoadSystem::LoadBoats(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["boats"] || !root["boats"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'boats' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["boats"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveAllBoat();
    }

    for (auto node : root["boats"]) {
        std::unique_ptr<Boat> boat = std::make_unique<Boat>(mGame);

        int startPlanetNum = node["startPlanet"] ? node["startPlanet"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[startPlanetNum];
        boat->SetCurrentPlanet(currentPlanet);

        int destPlanetNum = node["destPlanet"] ? node["destPlanet"].as<int>() : 0;
        Planet* destPlanet = mGame->GetCurrentStage()->GetPlanets()[destPlanetNum];
        boat->SetDestPlanet(destPlanet);

        int destStage = node["destStage"] ? node["destStage"].as<int>() : 0;
        boat->SetDestStage(destStage);

        float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
        boat->SetFacingYaw(facingYaw);

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        boat->SetPos(pos);

        YAML::Node boatRoot = YAML::LoadFile("../assets/data/actor/boats.yaml");
        for (auto boatNode : boatRoot["boats"]) {
            std::string modelPath = boatNode["modelPath"] ? boatNode["modelPath"].as<std::string>() : "";
            boat->SetModelPath(modelPath);

            float scale = boatNode["scale"] ? boatNode["scale"].as<float>() : 0.25f;
            boat->SetScale(glm::vec3(scale));
        }

        boat->Initialize();
        Boat* boat_ptr = boat.get();
        mGame->AddActor(std::move(boat));
        currentPlanet->AddBoat(boat_ptr);
    }
}

void ActorLoadSystem::LoadBoatParts(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["boatParts"] || !root["boatParts"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'boatParts' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["boatParts"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveAllBoatParts();
    }

    for (auto node : root["boatParts"]) {
        std::unique_ptr<BoatParts> boatParts = std::make_unique<BoatParts>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        boatParts->SetCurrentPlanet(currentPlanet);

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        boatParts->SetPos(pos);

        std::string type = node["type"] ? node["type"].as<std::string>() : "";
        YAML::Node boatPartsRoot = YAML::LoadFile("../assets/data/actor/boatparts.yaml");
        for (auto boatPartsNode : boatPartsRoot["boatParts"]) {
            if (boatPartsNode["type"].as<std::string>() == "common") {
                float scale = boatPartsNode["scale"] ? boatPartsNode["scale"].as<float>() : 0.25f;
                boatParts->SetScale(glm::vec3(scale));
            }

            if (type != boatPartsNode["type"].as<std::string>())
                continue;
            std::string modelPath = boatPartsNode["modelPath"] ? boatPartsNode["modelPath"].as<std::string>() : "";
            boatParts->SetModelPath(modelPath);
        }

        BoatParts* boatParts_ptr = boatParts.get();
        mGame->AddActor(std::move(boatParts));
        currentPlanet->AddBoatParts(boatParts_ptr);
        currentPlanet->Initialize();
    }
}

void ActorLoadSystem::LoadKeys(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["keys"] || !root["keys"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'keys' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["keys"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveKey();
    }

    for (auto node : root["keys"]) {
        std::unique_ptr<Key> key = std::make_unique<Key>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        key->SetCurrentPlanet(currentPlanet);

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        key->SetPos(pos);

        YAML::Node keyRoot = YAML::LoadFile("../assets/data/actor/keys.yaml");
        for (auto keyNode : keyRoot["keys"]) {
            std::string modelPath = keyNode["modelPath"] ? keyNode["modelPath"].as<std::string>() : "key.obj";
            key->SetModelPath(modelPath);

            float scale = keyNode["scale"] ? keyNode["scale"].as<float>() : 0.25f;
            key->SetScale(glm::vec3(scale));
        }

        Key* key_ptr = key.get();
        mGame->AddActor(std::move(key));
        currentPlanet->SetKey(key_ptr);
    }
}

void ActorLoadSystem::LoadCrystals(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["crystals"] || !root["crystals"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'crystals' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["crystals"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveAllCrystals();
    }

    for (auto node : root["crystals"]) {
        std::unique_ptr<Crystal> crystal = std::make_unique<Crystal>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        crystal->SetCurrentPlanet(currentPlanet);

        std::string type = node["type"] ? node["type"].as<std::string>() : "";
        YAML::Node crystalRoot = YAML::LoadFile("../assets/data/actor/crystals.yaml");
        for (auto crystalNode : crystalRoot["crystals"]) {
            if (crystalNode["type"].as<std::string>() == "common") {
                std::string modelPath = crystalNode["modelPath"] ? crystalNode["modelPath"].as<std::string>() : "";
                crystal->SetModelPath(modelPath);
                continue;
            }

            if (type != crystalNode["type"].as<std::string>())
                continue;

            float hp = crystalNode["hp"] ? crystalNode["hp"].as<float>() : 80.0f;
            crystal->GetDestructibleComponent()->SetDestroyHp(hp);

            float scale = crystalNode["scale"] ? crystalNode["scale"].as<float>() : 0.25f;
            crystal->SetScale(glm::vec3(scale));

            float radius = crystalNode["radius"] ? crystalNode["radius"].as<float>() : 1.0f;
            crystal->SetRadius(radius);
        }

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        crystal->SetPos(pos);

        Crystal* crystal_ptr = crystal.get();
        mGame->AddActor(std::move(crystal));
        currentPlanet->AddCrystal(crystal_ptr);
    }
}

void ActorLoadSystem::LoadStar(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["star"] || !root["star"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'star' sequence" << std::endl;
        return;
    }

    for (auto node : root["star"]) {
        std::unique_ptr<Star> star = std::make_unique<Star>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        star->SetCurrentPlanet(currentPlanet);

        YAML::Node starRoot = YAML::LoadFile("../assets/data/actor/stars.yaml");
        for (auto starNode : starRoot["stars"]) {

            std::string modelPath = starNode["modelPath"] ? starNode["modelPath"].as<std::string>() : "star.obj";
            star->SetModelPath(modelPath);

            float scale = starNode["scale"] ? starNode["scale"].as<float>() : 0.0f;
            star->SetScale(glm::vec3(scale));
        }

        Star* starPtr = star.get();
        mGame->AddActor(std::move(star));
        currentPlanet->SetStar(starPtr);
    }
}

void ActorLoadSystem::LoadPlatforms(const char* path)
{
    int currentPlanetNum = 0;
    YAML::Node root = YAML::LoadFile(path);
    if (!root["platforms"] || !root["platforms"].IsSequence()) {
        // std::cerr << "ActorLoadSystem: missing or invalid 'platforms' sequence" << std::endl;
        return;
    }
    for (const YAML::Node& node : root["platforms"]) {
        currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        currentPlanet->RemoveAllPlatforms();
    }

    for (auto node : root["platforms"]) {
        std::unique_ptr<Platform> platform = std::make_unique<Platform>(mGame);

        int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;
        Planet* currentPlanet = mGame->GetCurrentStage()->GetPlanets()[currentPlanetNum];
        platform->SetCurrentPlanet(currentPlanet);

        std::string type = node["type"] ? node["type"].as<std::string>() : "";
        YAML::Node platformRoot = YAML::LoadFile("../assets/data/actor/platforms.yaml");
        for (auto platformNode : platformRoot["platforms"]) {
            if (type != platformNode["type"].as<std::string>())
                continue;

            std::string modelPath = platformNode["modelPath"] ? platformNode["modelPath"].as<std::string>() : "";
            platform->SetModelPath(modelPath);

            if (platformNode["scale"]) {
                float scaleX = platformNode["scale"][0].as<float>();
                float scaleY = platformNode["scale"][1].as<float>();
                float scaleZ = platformNode["scale"][2].as<float>();
                glm::vec3 scale = glm::vec3(scaleX, scaleY, scaleZ);
                platform->SetScale(scale);
            }
        }

        glm::vec3 pos = CalculatePos(node, currentPlanet);
        platform->SetPos(pos);

        Platform* platformPtr = platform.get();
        mGame->AddActor(std::move(platform));
        currentPlanet->AddPlatform(platformPtr);
    }
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