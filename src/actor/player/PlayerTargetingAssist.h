#pragma once

class Enemy;
class Player;
class PlayerMovement;

class PlayerTargetingAssist {
public:
    static Enemy* FindAttackTarget(
        const Player& player,
        float attackRange,
        float attackAngle,
        bool requireAirborneTarget);

    static Enemy* FindAssistStrongTarget(
        const Player& player,
        float maxDistance,
        float attackAngle);
    static Enemy* FindNearestAirborneTargetOnCurrentPlanet(
        const Player& player);
    static Enemy* FindNearestAirborneTargetNearRecoveryOnCurrentPlanet(
        const Player& player,
        float maximumLaunchedTimerSeconds);

    static bool FaceTarget(Player& player, PlayerMovement& movement, const Enemy& target);
};
