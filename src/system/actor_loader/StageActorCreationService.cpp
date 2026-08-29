#include "StageActorCreationService.h"

#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/enemy/EnemyConfigLoader.h"
#include "actor/player/PlayerConfigLoader.h"
#include "actor/FallRespawnPoint.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "actor/TutorialTrigger.h"
#include "system/actor_loader/NpcStageConfigApplicator.h"
#include "system/actor_loader/PlatformStageConfig.h"
#include "system/actor_loader/PlatformStageConfigApplicator.h"
#include "system/actor_loader/StageActorFactory.h"
#include "system/actor_loader/StageBoatCreator.h"
#include "system/actor_loader/StagePlanetCreator.h"
#include "system/actor_loader/StagePlayerLoader.h"
#include "system/actor_loader/TutorialTriggerStageConfigApplicator.h"

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <utility>

StageActorCreationService::StageActorCreationService(
    StageActorFactory& actorFactory,
    StagePlayerLoader& playerLoader,
    StageBoatCreator& boatCreator,
    StagePlanetCreator& planetCreator)
    : mActorFactory(actorFactory),
      mPlayerLoader(playerLoader),
      mBoatCreator(boatCreator),
      mPlanetCreator(planetCreator)
{
}

bool StageActorCreationService::ReloadActorDefaultConfigs()
{
    try {
        YAML::Node boatArrivalPointConfig = YAML::LoadFile(
            "../assets/data/actor/boatarrivalpoints.yaml");
        YAML::Node boatPartsConfig = YAML::LoadFile(
            "../assets/data/actor/boatparts.yaml");
        YAML::Node crystalConfig = YAML::LoadFile(
            "../assets/data/actor/crystals.yaml");
        YAML::Node enemyConfig = YAML::LoadFile(
            "../assets/data/actor/enemies.yaml");
        YAML::Node fallRespawnPointConfig = YAML::LoadFile(
            "../assets/data/actor/fallrespawnpoints.yaml");
        YAML::Node keyConfig = YAML::LoadFile(
            "../assets/data/actor/keys.yaml");
        YAML::Node npcConfig = YAML::LoadFile(
            "../assets/data/actor/npcs.yaml");
        YAML::Node starConfig = YAML::LoadFile(
            "../assets/data/actor/stars.yaml");
        PlayerConfig playerConfig = PlayerConfigLoader::Load(
            "../assets/data/actor/players.yaml");

        mBoatArrivalPointConfig = std::move(boatArrivalPointConfig);
        mBoatPartsConfig = std::move(boatPartsConfig);
        mCrystalConfig = std::move(crystalConfig);
        mEnemyConfig = std::move(enemyConfig);
        mFallRespawnPointConfig = std::move(fallRespawnPointConfig);
        mKeyConfig = std::move(keyConfig);
        mNpcConfig = std::move(npcConfig);
        mStarConfig = std::move(starConfig);
        mPlayerLoader.SetPlayerConfig(std::move(playerConfig));
        return true;
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load actor default YAML: "
                  << exception.what() << '\n';
        return false;
    }
}

Player* StageActorCreationService::CreatePlayer(const YAML::Node& node, int playerNum)
{
    return mPlayerLoader.CreatePlayerFromStageNode(node, playerNum);
}

NPC* StageActorCreationService::CreateNPC(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<NPC>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "npc.obj", [](Planet* planet, NPC* npc) { planet->AddNPC(npc); },
        [this](NPC* npc, const YAML::Node& node) {
            const std::string type =
                node["type"] ? node["type"].as<std::string>() : "";
            npc->ApplyConfig(mNpcConfig, type);
            ApplyNpcStageConfig(*npc, node);
        },
        [](NPC* npc, const YAML::Node&) { npc->SetBaseScale(npc->GetScale()); });
}

TutorialTrigger* StageActorCreationService::CreateTutorialTrigger(
    const YAML::Node& node,
    int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<TutorialTrigger>(
        node,
        stageYamlIndex,
        1.0f,
        glm::vec3(2.0f),
        "selectField.obj",
        [](Planet* planet, TutorialTrigger* trigger) {
            planet->AddTutorialTrigger(trigger);
        },
        [](TutorialTrigger* trigger, const YAML::Node& triggerNode) {
            ApplyTutorialTriggerStageConfig(*trigger, triggerNode);
        },
        [](TutorialTrigger* trigger, const YAML::Node& triggerNode) {
            ApplyTutorialTriggerLegacyScale(*trigger, triggerNode);
        }
    );
}

Enemy* StageActorCreationService::CreateEnemy(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Enemy>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "enemy.obj",
        [](Planet* planet, Enemy* enemy) { planet->AddEnemy(enemy); },
        [this](Enemy* enemy, const YAML::Node& node) {
            const std::string type =
                node["type"] ? node["type"].as<std::string>() : "normal";
            enemy->ApplyConfig(
                EnemyConfigLoader::Parse(mEnemyConfig, type));
        },
        [](Enemy* enemy, const YAML::Node&) { enemy->SetBaseScale(enemy->GetScale()); });
}

Planet* StageActorCreationService::CreatePlanet(const YAML::Node& node)
{
    return mPlanetCreator.CreateFromStageNode(node);
}

Boat* StageActorCreationService::CreateBoat(
    const YAML::Node& node,
    int stageYamlIndex)
{
    return mBoatCreator.CreateFromStageNode(node, stageYamlIndex);
}

BoatParts* StageActorCreationService::CreateBoatParts(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<BoatParts>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, BoatParts* boatParts) {
            planet->AddBoatParts(boatParts);
            planet->Initialize();
        },
        [this](BoatParts* boatParts, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            boatParts->ApplyConfig(mBoatPartsConfig, type);
        });
}

Key* StageActorCreationService::CreateKey(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Key>(
        node, stageYamlIndex, 0.0f, glm::vec3(0.25f), "key.obj",
        [](Planet* planet, Key* key) { planet->SetKey(key); },
        [this](Key* key, const YAML::Node&) {
            key->ApplyConfig(mKeyConfig);
        });
}

Crystal* StageActorCreationService::CreateCrystal(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Crystal>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, Crystal* crystal) { planet->AddCrystal(crystal); },
        [this](Crystal* crystal, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            crystal->ApplyConfig(mCrystalConfig, type);
        });
}

Star* StageActorCreationService::CreateStar(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Star>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.0f), "star.obj",
        [](Planet* planet, Star* star) { planet->SetStar(star); },
        [this](Star* star, const YAML::Node&) {
            star->ApplyConfig(mStarConfig);
        },
        [](Star* star, const YAML::Node& node) {
            if (node["isActive"]) {
                star->SetIsActive(node["isActive"].as<bool>());
            }
        });
}

Platform* StageActorCreationService::CreatePlatform(const YAML::Node& node, int stageYamlIndex)
{
    Platform* platform = mActorFactory.CreatePlacedActorFromStageNode<Platform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, Platform* platform) { planet->AddPlatform(platform); },
        [stageYamlIndex](Platform* platform, const YAML::Node& node) {
            platform->SetPlatformId(
                node["platformId"]
                    ? node["platformId"].as<std::string>()
                    : "legacy_platforms_" +
                          std::to_string(stageYamlIndex));
            if (node["ugcGeneratedPlatform"] &&
                node["ugcGeneratedPlatform"].as<bool>(false)) {
                platform->SetUGCGeneratedLayer(
                    node["ugcGridLayer"]
                        ? node["ugcGridLayer"].as<int>()
                        : 0);
            }
            ApplyPlatformStageConfig(
                *platform,
                ParsePlatformStageConfig(node));
        });
    if (platform) {
        platform->SetStageSequenceName("platforms");
    }
    return platform;
}

Platform* StageActorCreationService::CreateLegacyMovingPlatform(
    const YAML::Node& node,
    int stageYamlIndex)
{
    Platform* platform = mActorFactory.CreatePlacedActorFromStageNode<Platform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, Platform* platform) { planet->AddPlatform(platform); },
        [stageYamlIndex](Platform* platform, const YAML::Node& node) {
            platform->SetPlatformId(
                node["platformId"]
                    ? node["platformId"].as<std::string>()
                    : "legacy_movingPlatforms_" +
                          std::to_string(stageYamlIndex));
            ApplyPlatformStageConfig(
                *platform,
                ParsePlatformStageConfig(node, true));
        });
    if (platform) {
        platform->SetStageSequenceName("movingPlatforms");
    }
    return platform;
}

JewelItem* StageActorCreationService::CreateJewelItem(
    const YAML::Node& node,
    int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<JewelItem>(
        node,
        stageYamlIndex,
        0.15f,
        glm::vec3(0.22f),
        "crystal.obj",
        [](Planet* planet, JewelItem* jewelItem) {
            planet->AddJewelItem(jewelItem);
        });
}

HazardActor* StageActorCreationService::CreateHazardActor(
    const YAML::Node& node,
    int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<HazardActor>(
        node,
        stageYamlIndex,
        0.75f,
        glm::vec3(0.75f),
        "crystal.obj",
        [](Planet* planet, HazardActor* hazardActor) {
            planet->AddHazardActor(hazardActor);
        },
        [](HazardActor* hazardActor, const YAML::Node& actorNode) {
            if (actorNode["damage"]) {
                hazardActor->SetDamage(actorNode["damage"].as<float>());
            }
            if (actorNode["triggerRadius"]) {
                hazardActor->SetTriggerRadius(
                    actorNode["triggerRadius"].as<float>());
            }
            if (actorNode["damageIntervalSeconds"]) {
                hazardActor->SetDamageIntervalSeconds(
                    actorNode["damageIntervalSeconds"].as<float>());
            }
        });
}

BoatArrivalPoint* StageActorCreationService::CreateBoatArrivalPoint(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<BoatArrivalPoint>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.4f), "platform.obj",
        [](Planet* planet, BoatArrivalPoint* point) { planet->AddBoatArrivalPoint(point); },
        [this](BoatArrivalPoint* point, const YAML::Node&) {
            point->ApplyConfig(mBoatArrivalPointConfig);
        });
}

FallRespawnPoint* StageActorCreationService::CreateFallRespawnPoint(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<FallRespawnPoint>(
        node, stageYamlIndex, 0.0f, glm::vec3(4.0f, 1.0f, 4.0f), "platform.obj",
        [](Planet* planet, FallRespawnPoint* point) { planet->AddFallRespawnPoint(point); },
        [this](FallRespawnPoint* point, const YAML::Node&) {
            point->ApplyConfig(mFallRespawnPointConfig);
        },
        [](FallRespawnPoint* point, const YAML::Node& node) {
            if (node["damage"]) {
                point->SetDamage(node["damage"].as<float>());
            }
        });
}

StageObject* StageActorCreationService::CreateStageObject(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<StageObject>(
        node, stageYamlIndex, 1.0f, glm::vec3(1.0f), "",
        [](Planet* planet, StageObject* stageObject) { planet->AddStageObject(stageObject); },
        [](StageObject* stageObject, const YAML::Node& node) {
            const bool collisionEnabled = node["collision"] ? node["collision"].as<bool>() : true;
            stageObject->SetCollisionEnabled(collisionEnabled);
        });
}
