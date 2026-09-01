#pragma once

#include <glm/glm.hpp>
#include <string>

class Actor;

struct StageActorPlacement {
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
};

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
    TutorialTrigger,
    JewelItem,
    HazardActor
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
