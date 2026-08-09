#pragma once

#include <string>

class Actor;

enum class StageActorType {
    Planet,
    Enemy,
    Platform,
    Crystal,
    NPC,
    BoatParts,
    Boat,
    BoatArrivalPoint,
    FallRespawnPoint,
    Key,
    Star,
    StageObject,
    TutorialTrigger
};

struct StageActorRef {
    StageActorType type;
    int yamlIndex = -1;
    std::string sequenceName;
    std::string label;
};

struct StageActorInstance {
    Actor* actor = nullptr;
    StageActorRef ref;
};

struct StageActorTypeInfo {
    StageActorType type;
    std::string sequenceName;
    std::string displayName;
};
