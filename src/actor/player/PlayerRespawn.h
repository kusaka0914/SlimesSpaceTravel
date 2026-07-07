#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerStateMachine;
class PlayerStatus;

class PlayerRespawn {
public:
    void ApplyFallDamageAndRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat,
                                   PlayerStatus& status, float damage);
    void Respawn(Player& player);
    void Restart(Player& player, PlayerStateMachine& stateMachine, PlayerStatus& status);
    bool IsFallIntoPlanetInside(const Player& player) const;
    void CheckFallRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat, PlayerStatus& status,
                          const glm::vec3& prevPos);

    void SetRestartPlanetIndex(int restartPlanetIndex) { mRestartPlanetIndex = restartPlanetIndex; }
    void SetRestartPos(const glm::vec3& restartPos) { mRestartPos = restartPos; }

    int GetRestartPlanetIndex() const { return mRestartPlanetIndex; }
    const glm::vec3& GetRestartPos() const { return mRestartPos; }

private:
    int mRestartPlanetIndex = 0;
    glm::vec3 mRestartPos = glm::vec3(0.0f);
};
