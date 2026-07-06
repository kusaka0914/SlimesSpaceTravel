#pragma once

#include <glm/glm.hpp>

class Boat;
class PlayerCombat;
struct PlayerModuleContext;

class PlayerMovement {
public:
    bool isDodged = true;

    int currentPlanetNum = 0;
    int playerNum = 1;

    float dodgeTimer = 0.0f;
    float dodgeDuration = 0.1f;
    float dodgeCooldown = 0.0f;
    float dodgeCooldownTime = 0.3f;
    float dodgeDistance = 3.0f;
    float dodgeStartHeight = 0.0f;
    float moveSpeed = 10.2f;
    float chargeMoveSpeed = 6.0f;
    float knockBackSpeed = 0.0f;

    glm::vec3 forwardVec = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 leftVec = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 knockBackFrom = glm::vec3(0.0f);
    glm::vec3 dodgeDir = glm::vec3(0.0f);

    bool CanWalk(const PlayerCombat& combat) const;

    void UpdateWorldVec(PlayerModuleContext& context);
    void UpdateWalk(PlayerModuleContext& context, float deltaTime);
    void UpdateBoatRide(PlayerModuleContext& context);
    void ChangeFaceDir(PlayerModuleContext& context);
    void UpdateFacingForwardVec(PlayerModuleContext& context);
    void MoveDuringDodging(PlayerModuleContext& context, float deltaTime);
    void MoveDuringAttacking(PlayerModuleContext& context, float deltaTime);
    void MoveDuringCharging(PlayerModuleContext& context, float deltaTime);
    void MoveDuringStrongAttacking(PlayerModuleContext& context, float deltaTime);
    void MoveDuringKnockBack(PlayerModuleContext& context, float deltaTime);
    void FollowMovingBoat(PlayerModuleContext& context, Boat* boat);
    bool IsTouchingBoat(PlayerModuleContext& context, Boat* boat);
    void StartDodging(PlayerModuleContext& context);
    void StartJumping(PlayerModuleContext& context, float deltaTime);
    void StartRidingBoat(PlayerModuleContext& context, Boat* boat);
    void OnBoatArrived(PlayerModuleContext& context, Boat* boat);
    void OnLanded(PlayerModuleContext& context);
    void OnUpVecUpdateFailed(PlayerModuleContext& context);
    void OnCastSucceeded(PlayerModuleContext& context);
    void SnapToGround(PlayerModuleContext& context, float upOffset, float downLength);
};
