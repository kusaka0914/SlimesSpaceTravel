#pragma once

#include <string>

enum class StageActorType { Enemy, Platform, Crystal, NPC, BoatParts, Boat, Key, Star };

struct StageActorRef {
    StageActorType type;
    int yamlIndex = -1;
    std::string sequenceName;
    std::string label;
};