#pragma once

#include <glm/glm.hpp>

class Planet;
class Stage;
class Actor;

class StageActorPlanetBindingService {
public:


    static bool RefreshNearestPlanetBindings(Stage* stage);
    static bool RefreshNearestPlanetBinding(Stage* stage, Actor* actor);



    static void TranslateActorsBoundToPlanet(
        Planet* planet,
        const glm::vec3& translation);




    static void PreserveBoundActorWorldPositionsAfterPlanetMove(
        Planet* planet,
        const glm::vec3& planetTranslation);



    static void ReprojectSurfaceActorsAfterPlanetScale(Planet* planet);

};
