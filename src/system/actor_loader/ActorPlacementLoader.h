#pragma once

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Actor;
class Planet;

class ActorPlacementLoader {
public:
    glm::vec3 CalculatePos(const YAML::Node& node, const Planet& currentPlanet) const;

    void ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet,
                                     int stageYamlIndex, float defaultHeight = 0.0f) const;
    void ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node) const;
    void ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node) const;
};
