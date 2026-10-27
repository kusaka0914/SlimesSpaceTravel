#include "system/actor_loader/StageActorLocator.h"

#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/CameraZone.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
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
#include "world/Stage.h"

Actor* StageActorLocator::FindPlacedActor(const Stage& stage, const std::string& sequenceName, int stageYamlIndex) const
{
    if (stageYamlIndex < 0) {
        return nullptr;
    }

    if (sequenceName == "planets") {
        const auto& planets = stage.GetPlanets();
        return stageYamlIndex < static_cast<int>(planets.size()) ? planets[stageYamlIndex] : nullptr;
    }

    const auto findByIndex = [stageYamlIndex](const auto& actors) -> Actor* {
        for (Actor* actor : actors) {
            if (actor && actor->GetStageYamlIndex() == stageYamlIndex) {
                return actor;
            }
        }
        return nullptr;
    };

    for (Planet* planet : stage.GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform || platform->GetStageYamlIndex() != stageYamlIndex) {
                continue;
            }

            // 種別名が未設定の足場は、エディターと同じく通常のplatformsとして解決する。
            const std::string& platformSequenceName = platform->GetStageSequenceName();
            if (platformSequenceName == sequenceName || (platformSequenceName.empty() && sequenceName == "platforms")) {
                return platform;
            }
        }

        Actor* matchingActor = nullptr;
        if (sequenceName == "enemies") {
            matchingActor = findByIndex(planet->GetEnemies());
        } else if (sequenceName == "boats") {
            matchingActor = findByIndex(planet->GetBoats());
        } else if (sequenceName == "boatParts") {
            matchingActor = findByIndex(planet->GetBoatParts());
        } else if (sequenceName == "crystals") {
            matchingActor = findByIndex(planet->GetCrystals());
        } else if (sequenceName == "NPCs") {
            matchingActor = findByIndex(planet->GetNPCs());
        } else if (sequenceName == "tutorialTriggers") {
            matchingActor = findByIndex(planet->GetTutorialTriggers());
        } else if (sequenceName == "jewelItems") {
            matchingActor = findByIndex(planet->GetJewelItems());
        } else if (sequenceName == "hazardActors") {
            matchingActor = findByIndex(planet->GetHazardActors());
        } else if (sequenceName == "boatArrivalPoints") {
            matchingActor = findByIndex(planet->GetBoatArrivalPoints());
        } else if (sequenceName == "fallRespawnPoints") {
            matchingActor = findByIndex(planet->GetFallRespawnPoints());
        } else if (sequenceName == "stageObjects" || sequenceName == "cameraZones") {
            for (auto* actor : planet->GetStageObjects()) {
                const bool isZone = dynamic_cast<CameraZone*>(actor) != nullptr;
                if (actor && actor->GetStageYamlIndex() == stageYamlIndex &&
                    isZone == (sequenceName == "cameraZones")) {
                    matchingActor = actor;
                    break;
                }
            }
        } else if (sequenceName == "keys") {
            Key* key = planet->GetKey();
            if (key && key->GetStageYamlIndex() == stageYamlIndex) {
                matchingActor = key;
            }
        } else if (sequenceName == "star") {
            Star* star = planet->GetStar();
            if (star && star->GetStageYamlIndex() == stageYamlIndex) {
                matchingActor = star;
            }
        }

        if (matchingActor) {
            return matchingActor;
        }
    }

    return nullptr;
}

Platform* StageActorLocator::FindPlacedPlatform(const Stage& stage, const std::string& platformId,
                                                int preferredStageYamlIndex) const
{
    if (platformId.empty()) {
        return nullptr;
    }

    Platform* matchingPlatform = nullptr;
    for (Planet* planet : stage.GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform || platform->GetPlatformId() != platformId) {
                continue;
            }
            if (platform->GetStageYamlIndex() == preferredStageYamlIndex) {
                return platform;
            }
            if (!matchingPlatform) {
                matchingPlatform = platform;
            }
        }
    }
    return matchingPlatform;
}
