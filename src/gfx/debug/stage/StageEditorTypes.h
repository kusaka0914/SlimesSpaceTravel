#pragma once

#include <string>

class Actor;

enum class StageActorType {
    Enemy,
    Platform,
    MovingPlatform,
    Crystal,
    NPC,
    BoatParts,
    Boat,
    BoatArrivalPoint,
    Key,
    Star
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
    const char* sequenceName;
    const char* displayName;
};