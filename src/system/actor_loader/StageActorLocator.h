#pragma once

#include <string>

class Actor;
class Platform;
class Stage;

class StageActorLocator {
public:
    Actor* FindPlacedActor(
        const Stage& stage,
        const std::string& sequenceName,
        int stageYamlIndex) const;
    Platform* FindPlacedPlatform(
        const Stage& stage,
        const std::string& platformId,
        int preferredStageYamlIndex = -1) const;
};
