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

    // Keeps every owned actor at its current world position while rewriting
    // its planet-local placement for a planet-center edit. This is required
    // for PlanetOnly editor moves to survive a stage reload.
    static void PreserveBoundActorWorldPositionsAfterPlanetMove(
        Planet* planet,
        const glm::vec3& planetTranslation);

    // Reprojects actors whose placement is expressed by spherical coordinates.
    // Platforms and stage geometry retain their authored local coordinates.
    static void ReprojectSurfaceActorsAfterPlanetScale(Planet* planet);

};
