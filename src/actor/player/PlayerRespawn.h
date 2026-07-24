#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerStateMachine;
class PlayerStatus;

class PlayerRespawn {
public:
    void ApplyFallDamageAndRespawn(Player& player, PlayerStatus& status, float damage);
    void Respawn(Player& player);
    bool IsFallIntoPlanetInside(const Player& player) const;
    void CheckFallRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat, PlayerStatus& status,
                          const glm::vec3& prevPos);
    bool UpdateMissingGroundRayRespawn(Player& player, const PlayerStatus& status, float deltaTime);
    void OnGroundRayCastSucceeded() { mGroundRayHitThisFrame = true; }
    void OnRespawnCompleted();

    void SetRestartPlanetIndex(int restartPlanetIndex) { mRestartPlanetIndex = restartPlanetIndex; }
    void SetRestartPos(const glm::vec3& restartPos) { mRestartPos = restartPos; }

    int GetRestartPlanetIndex() const { return mRestartPlanetIndex; }
    const glm::vec3& GetRestartPos() const { return mRestartPos; }

private:
    static constexpr float missingGroundRayRespawnDelay = 5.0f;

    int mRestartPlanetIndex = 0;
    glm::vec3 mRestartPos = glm::vec3(0.0f);
    float mMissingGroundRayDuration = 0.0f;
    bool mGroundRayHitThisFrame = false;
    bool mRespawnFadeRequested = false;
};
