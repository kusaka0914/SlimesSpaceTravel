#pragma once

#include <glm/glm.hpp>

class Planet;
class Stage;
class Actor;

class StageActorPlanetBindingService {
public:
    // Reconciles both Actor::currentPlanet and the owning planet registry.
    // World positions are preserved when ownership changes.
    static bool RefreshNearestPlanetBindings(Stage* stage);
    static bool RefreshNearestPlanetBinding(Stage* stage, Actor* actor);

    // Moves every actor currently owned by the planet by the same world-space
    // delta. Relative placement and moving-platform paths remain unchanged.
    static void TranslateActorsBoundToPlanet(
        Planet* planet,
        const glm::vec3& translation);

    // Reprojects actors whose placement is expressed by spherical coordinates.
    // Platforms and stage geometry retain their authored local coordinates.
    static void ReprojectSurfaceActorsAfterPlanetScale(Planet* planet);

};
