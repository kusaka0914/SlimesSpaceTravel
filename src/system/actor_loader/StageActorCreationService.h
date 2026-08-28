#pragma once

#include <yaml-cpp/yaml.h>

class Boat;
class BoatArrivalPoint;
class BoatParts;
class Crystal;
class Enemy;
class FallRespawnPoint;
class HazardActor;
class JewelItem;
class Key;
class NPC;
class Planet;
class Platform;
class Player;
class StageActorFactory;
class StageBoatCreator;
class StageObject;
class StagePlanetCreator;
class StagePlayerLoader;
class Star;
class TutorialTrigger;

class StageActorCreationService {
public:
    StageActorCreationService(
        StageActorFactory& actorFactory,
        StagePlayerLoader& playerLoader,
        StageBoatCreator& boatCreator,
        StagePlanetCreator& planetCreator);

    Planet* CreatePlanet(const YAML::Node& node);
    Player* CreatePlayer(const YAML::Node& node, int playerNum);
    Enemy* CreateEnemy(const YAML::Node& node, int stageYamlIndex);
    Platform* CreatePlatform(const YAML::Node& node, int stageYamlIndex);
    Platform* CreateLegacyMovingPlatform(
        const YAML::Node& node,
        int stageYamlIndex);
    NPC* CreateNPC(const YAML::Node& node, int stageYamlIndex);
    Crystal* CreateCrystal(const YAML::Node& node, int stageYamlIndex);
    BoatParts* CreateBoatParts(const YAML::Node& node, int stageYamlIndex);
    Boat* CreateBoat(const YAML::Node& node, int stageYamlIndex);
    Star* CreateStar(const YAML::Node& node, int stageYamlIndex);
    Key* CreateKey(const YAML::Node& node, int stageYamlIndex);
    BoatArrivalPoint* CreateBoatArrivalPoint(
        const YAML::Node& node,
        int stageYamlIndex);
    FallRespawnPoint* CreateFallRespawnPoint(
        const YAML::Node& node,
        int stageYamlIndex);
    StageObject* CreateStageObject(
        const YAML::Node& node,
        int stageYamlIndex);
    TutorialTrigger* CreateTutorialTrigger(
        const YAML::Node& node,
        int stageYamlIndex);
    JewelItem* CreateJewelItem(
        const YAML::Node& node,
        int stageYamlIndex);
    HazardActor* CreateHazardActor(
        const YAML::Node& node,
        int stageYamlIndex);

private:
    StageActorFactory& mActorFactory;
    StagePlayerLoader& mPlayerLoader;
    StageBoatCreator& mBoatCreator;
    StagePlanetCreator& mPlanetCreator;
};
