#include "system/StageActorPlanetBindingService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/StageObject.h"
#include "actor/TutorialTrigger.h"
#include "component/PlatformMovementComponent.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

struct RegisteredActorInfo {
    Actor* actor = nullptr;
    Planet* registeredPlanet = nullptr;
    bool isRegisteredToMultiplePlanets = false;
};

void RecordActor(
    std::unordered_map<Actor*, RegisteredActorInfo>& registeredActors,
    Actor* actor,
    Planet* registeredPlanet)
{
    if (!actor || !registeredPlanet) {
        return;
    }

    const auto [iterator, wasInserted] = registeredActors.emplace(
        actor,
        RegisteredActorInfo{actor, registeredPlanet, false});
    if (!wasInserted && iterator->second.registeredPlanet != registeredPlanet) {
        iterator->second.isRegisteredToMultiplePlanets = true;
    }
}

template <class TActor>
void RecordActors(
    std::unordered_map<Actor*, RegisteredActorInfo>& registeredActors,
    const std::vector<TActor*>& actors,
    Planet* registeredPlanet)
{
    for (TActor* actor : actors) {
        RecordActor(registeredActors, actor, registeredPlanet);
    }
}

std::unordered_map<Actor*, RegisteredActorInfo> CollectRegisteredActors(
    const std::vector<Planet*>& planets)
{
    std::unordered_map<Actor*, RegisteredActorInfo> registeredActors;

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        RecordActors(registeredActors, planet->GetEnemies(), planet);
        RecordActors(registeredActors, planet->GetBoats(), planet);
        RecordActors(registeredActors, planet->GetBoatParts(), planet);
        RecordActors(registeredActors, planet->GetCrystals(), planet);
        RecordActors(registeredActors, planet->GetNPCs(), planet);
        RecordActors(registeredActors, planet->GetPlatforms(), planet);
        RecordActors(registeredActors, planet->GetBoatArrivalPoints(), planet);
        RecordActors(registeredActors, planet->GetFallRespawnPoints(), planet);
        RecordActors(registeredActors, planet->GetStageObjects(), planet);
        RecordActors(registeredActors, planet->GetTutorialTriggers(), planet);
        RecordActor(registeredActors, planet->GetKey(), planet);
        RecordActor(registeredActors, planet->GetStar(), planet);
    }

    return registeredActors;
}

float CalculateDistanceToPlanetSurface(
    const Planet& planet,
    const glm::vec3& worldPosition)
{
    switch (planet.GetPlanetShape()) {
    case Planet::PlanetShape::Ellipse:
        return planet.CalculateEllipseSurfaceProjection(worldPosition).distance;
    case Planet::PlanetShape::Sphere:
        return std::abs(
            glm::length(worldPosition - planet.GetPos()) -
            std::abs(planet.GetRadius()));
    case Planet::PlanetShape::Normal:
    default:
        return glm::length(worldPosition - planet.GetPos());
    }
}

Planet* FindNearestPlanet(
    const std::vector<Planet*>& planets,
    const glm::vec3& worldPosition,
    Planet* currentPlanet)
{
    constexpr float distanceComparisonEpsilon = 0.001f;

    Planet* nearestPlanet = nullptr;
    float nearestDistance = std::numeric_limits<float>::max();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        const float distance =
            CalculateDistanceToPlanetSurface(*planet, worldPosition);
        const bool isMeaningfullyCloser =
            distance + distanceComparisonEpsilon < nearestDistance;
        const bool isEquivalentCurrentPlanet =
            currentPlanet == planet &&
            std::abs(distance - nearestDistance) <=
                distanceComparisonEpsilon;

        if (isMeaningfullyCloser || isEquivalentCurrentPlanet) {
            nearestPlanet = planet;
            nearestDistance = distance;
        }
    }

    return nearestPlanet;
}

void RemoveActorFromPlanet(Planet& planet, Actor* actor)
{
    if (TutorialTrigger* trigger = dynamic_cast<TutorialTrigger*>(actor)) {
        planet.RemoveTutorialTrigger(trigger);
    } else if (Enemy* enemy = dynamic_cast<Enemy*>(actor)) {
        planet.RemoveEnemy(enemy);
    } else if (Boat* boat = dynamic_cast<Boat*>(actor)) {
        planet.RemoveBoat(boat);
    } else if (BoatParts* boatParts = dynamic_cast<BoatParts*>(actor)) {
        planet.RemoveBoatParts(boatParts);
    } else if (Crystal* crystal = dynamic_cast<Crystal*>(actor)) {
        planet.RemoveCrystal(crystal);
    } else if (NPC* npc = dynamic_cast<NPC*>(actor)) {
        planet.RemoveNPC(npc);
    } else if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        planet.RemovePlatform(platform);
    } else if (BoatArrivalPoint* point =
                   dynamic_cast<BoatArrivalPoint*>(actor)) {
        planet.RemoveBoatArrivalPoint(point);
    } else if (FallRespawnPoint* point =
                   dynamic_cast<FallRespawnPoint*>(actor)) {
        planet.RemoveFallRespawnPoint(point);
    } else if (StageObject* stageObject =
                   dynamic_cast<StageObject*>(actor)) {
        planet.RemoveStageObject(stageObject);
    } else if (Key* key = dynamic_cast<Key*>(actor)) {
        if (planet.GetKey() == key) {
            planet.RemoveKey();
        }
    } else if (Star* star = dynamic_cast<Star*>(actor)) {
        if (planet.GetStar() == star) {
            planet.RemoveStar();
        }
    }
}

void AddActorToPlanet(Planet& planet, Actor* actor)
{
    if (TutorialTrigger* trigger = dynamic_cast<TutorialTrigger*>(actor)) {
        planet.AddTutorialTrigger(trigger);
    } else if (Enemy* enemy = dynamic_cast<Enemy*>(actor)) {
        planet.AddEnemy(enemy);
    } else if (Boat* boat = dynamic_cast<Boat*>(actor)) {
        planet.AddBoat(boat);
    } else if (BoatParts* boatParts = dynamic_cast<BoatParts*>(actor)) {
        planet.AddBoatParts(boatParts);
    } else if (Crystal* crystal = dynamic_cast<Crystal*>(actor)) {
        planet.AddCrystal(crystal);
    } else if (NPC* npc = dynamic_cast<NPC*>(actor)) {
        planet.AddNPC(npc);
    } else if (Platform* platform = dynamic_cast<Platform*>(actor)) {
        planet.AddPlatform(platform);
    } else if (BoatArrivalPoint* point =
                   dynamic_cast<BoatArrivalPoint*>(actor)) {
        planet.AddBoatArrivalPoint(point);
    } else if (FallRespawnPoint* point =
                   dynamic_cast<FallRespawnPoint*>(actor)) {
        planet.AddFallRespawnPoint(point);
    } else if (StageObject* stageObject =
                   dynamic_cast<StageObject*>(actor)) {
        planet.AddStageObject(stageObject);
    } else if (Key* key = dynamic_cast<Key*>(actor)) {
        planet.SetKey(key);
    } else if (Star* star = dynamic_cast<Star*>(actor)) {
        planet.SetStar(star);
    }
}

void UpdateSphericalPlacement(Actor& actor, const Planet& planet)
{
    constexpr float positionEpsilon = 0.000001f;

    const glm::vec3 planetOffset = actor.GetPos() - planet.GetPos();
    const float centerDistance = glm::length(planetOffset);
    if (centerDistance <= positionEpsilon) {
        return;
    }

    const glm::vec3 radialDirection = planetOffset / centerDistance;
    const float theta =
        std::atan2(radialDirection.z, radialDirection.x);
    const float phi =
        std::asin(glm::clamp(radialDirection.y, -1.0f, 1.0f));

    float height = centerDistance - std::abs(planet.GetRadius());
    if (planet.GetPlanetShape() == Planet::PlanetShape::Ellipse) {
        const Planet::EllipseSurfaceProjection projection =
            planet.CalculateEllipseSurfaceProjection(actor.GetPos());
        height = projection.isOutside
            ? projection.distance
            : -projection.distance;
    }

    actor.SetSphericalPlacement(theta, phi, height);
}

void PreserveMovingPlatformWorldPath(
    Actor& actor,
    const Planet* previousPlanet,
    const Planet& nextPlanet)
{
    Platform* platform = dynamic_cast<Platform*>(&actor);
    if (!platform || !platform->GetMovementComponent()) {
        return;
    }

    PlatformMovementComponent* movement =
        platform->GetMovementComponent();
    const glm::vec3 previousPlanetCenter =
        previousPlanet ? previousPlanet->GetPos() : glm::vec3(0.0f);
    const glm::vec3 worldPathStart =
        previousPlanetCenter + movement->GetBaseLocalPos();
    movement->SetBaseLocalPos(worldPathStart - nextPlanet.GetPos());
}

void RefreshBoatDestinations(const std::vector<Planet*>& planets)
{
    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }
        for (Boat* boat : planet->GetBoats()) {
            if (boat) {
                boat->RefreshDestination();
            }
        }
    }
}

std::vector<Actor*> CollectActorsOwnedByPlanet(Planet& planet)
{
    std::unordered_set<Actor*> uniqueActors;

    const auto addActors = [&uniqueActors](const auto& actors) {
        for (Actor* actor : actors) {
            if (actor) {
                uniqueActors.insert(actor);
            }
        }
    };

    addActors(planet.GetEnemies());
    addActors(planet.GetBoats());
    addActors(planet.GetBoatParts());
    addActors(planet.GetCrystals());
    addActors(planet.GetNPCs());
    addActors(planet.GetPlatforms());
    addActors(planet.GetBoatArrivalPoints());
    addActors(planet.GetFallRespawnPoints());
    addActors(planet.GetStageObjects());
    addActors(planet.GetTutorialTriggers());
    if (planet.GetKey()) {
        uniqueActors.insert(planet.GetKey());
    }
    if (planet.GetStar()) {
        uniqueActors.insert(planet.GetStar());
    }

    return std::vector<Actor*>(uniqueActors.begin(), uniqueActors.end());
}

bool RefreshActorBinding(
    const std::vector<Planet*>& planets,
    const RegisteredActorInfo& registration,
    std::unordered_set<Planet*>& affectedPlanets)
{
    Actor* actor = registration.actor;
    if (!actor) {
        return false;
    }

    Planet* nearestPlanet = FindNearestPlanet(
        planets,
        actor->GetPos(),
        actor->GetCurrentPlanet());
    if (!nearestPlanet) {
        return false;
    }

    const bool isAlreadyConsistent =
        actor->GetCurrentPlanet() == nearestPlanet &&
        registration.registeredPlanet == nearestPlanet &&
        !registration.isRegisteredToMultiplePlanets;
    if (isAlreadyConsistent) {
        return false;
    }

    Planet* previousPlanet = actor->GetCurrentPlanet();
    PreserveMovingPlatformWorldPath(
        *actor,
        previousPlanet,
        *nearestPlanet);

    for (Planet* planet : planets) {
        if (planet) {
            RemoveActorFromPlanet(*planet, actor);
        }
    }

    AddActorToPlanet(*nearestPlanet, actor);
    actor->SetCurrentPlanet(nearestPlanet);
    UpdateSphericalPlacement(*actor, *nearestPlanet);
    actor->CaptureEditorAuthoredPosition();

    if (previousPlanet) {
        affectedPlanets.insert(previousPlanet);
    }
    affectedPlanets.insert(nearestPlanet);
    return true;
}

void FinalizeBindingChanges(
    const std::vector<Planet*>& planets,
    const std::unordered_set<Planet*>& affectedPlanets)
{
    for (Planet* planet : affectedPlanets) {
        planet->Initialize();
    }
    RefreshBoatDestinations(planets);
}

} // namespace

bool StageActorPlanetBindingService::RefreshNearestPlanetBindings(
    Stage* stage)
{
    if (!stage) {
        return false;
    }

    const std::vector<Planet*>& planets = stage->GetPlanets();
    if (planets.empty()) {
        return false;
    }

    const auto registeredActors = CollectRegisteredActors(planets);
    std::unordered_set<Planet*> affectedPlanets;

    for (const auto& [actorPointer, registration] : registeredActors) {
        (void)actorPointer;
        RefreshActorBinding(planets, registration, affectedPlanets);
    }

    if (affectedPlanets.empty()) {
        return false;
    }

    FinalizeBindingChanges(planets, affectedPlanets);
    return true;
}

bool StageActorPlanetBindingService::RefreshNearestPlanetBinding(
    Stage* stage,
    Actor* actor)
{
    if (!stage || !actor) {
        return false;
    }

    const std::vector<Planet*>& planets = stage->GetPlanets();
    const auto registeredActors = CollectRegisteredActors(planets);
    const auto registration = registeredActors.find(actor);
    if (registration == registeredActors.end()) {
        return false;
    }

    std::unordered_set<Planet*> affectedPlanets;
    if (!RefreshActorBinding(
            planets,
            registration->second,
            affectedPlanets)) {
        return false;
    }

    FinalizeBindingChanges(planets, affectedPlanets);
    return true;
}

void StageActorPlanetBindingService::TranslateActorsBoundToPlanet(
    Planet* planet,
    const glm::vec3& translation)
{
    if (!planet || glm::length(translation) <= 0.000001f) {
        return;
    }

    for (Actor* actor : CollectActorsOwnedByPlanet(*planet)) {
        actor->SetPos(actor->GetPos() + translation);
    }

    Game* game = planet->GetGame();
    if (game) {
        for (Player* player : game->GetPlayers()) {
            if (player && player->GetCurrentPlanet() == planet) {
                player->SetPos(player->GetPos() + translation);
            }
        }
    }

    Stage* stage = planet->GetCurrentStage();
    if (stage) {
        RefreshBoatDestinations(stage->GetPlanets());
    }
}

void StageActorPlanetBindingService::ReprojectSurfaceActorsAfterPlanetScale(
    Planet* planet)
{
    if (!planet) {
        return;
    }

    const auto reprojectActor = [planet](Actor* actor) {
        if (!actor) {
            return;
        }

        actor->SetPos(
            planet->CalculateSurfacePos(
                actor->GetTheta(),
                actor->GetPhi(),
                actor->GetHeight()));
        actor->CaptureEditorAuthoredPosition();
    };

    for (Enemy* enemy : planet->GetEnemies()) {
        reprojectActor(enemy);
    }
    for (Boat* boat : planet->GetBoats()) {
        reprojectActor(boat);
    }
    for (BoatParts* boatParts : planet->GetBoatParts()) {
        reprojectActor(boatParts);
    }
    for (Crystal* crystal : planet->GetCrystals()) {
        reprojectActor(crystal);
    }
    for (NPC* npc : planet->GetNPCs()) {
        reprojectActor(npc);
    }
    for (TutorialTrigger* trigger : planet->GetTutorialTriggers()) {
        reprojectActor(trigger);
    }
    reprojectActor(planet->GetKey());
    reprojectActor(planet->GetStar());

    Game* game = planet->GetGame();
    if (game) {
        for (Player* player : game->GetPlayers()) {
            if (player && player->GetCurrentPlanet() == planet) {
                player->SetPos(
                    planet->CalculateSurfacePos(
                        player->GetTheta(),
                        player->GetPhi(),
                        player->GetHeight()));
            }
        }
    }

    Stage* stage = planet->GetCurrentStage();
    if (stage) {
        RefreshBoatDestinations(stage->GetPlanets());
    }
}
