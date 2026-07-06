#pragma once

#include <glm/glm.hpp>

struct PlayerModuleContext;

class PlayerRespawn {
public:
    int restartPlanetIndex = 0;
    glm::vec3 restartPos = glm::vec3(0.0f);

    void ApplyFallDamageAndRespawn(PlayerModuleContext& context, float damage);
    void Respawn(PlayerModuleContext& context);
    void Restart(PlayerModuleContext& context);
    bool IsFallIntoPlanetInside(PlayerModuleContext& context);
    void CheckFallRespawn(PlayerModuleContext& context, const glm::vec3& prevPos);
};
