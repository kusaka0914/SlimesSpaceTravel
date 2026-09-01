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
        bool requireLaunchedTarget);

    static Enemy* FindAssistStrongTarget(
        const Player& player,
        float maxDistance,
        float attackAngle);
    static Enemy* FindNearestLaunchedTargetOnCurrentPlanet(
        const Player& player);
    static Enemy* FindNearestLaunchedTargetNearRecoveryOnCurrentPlanet(
        const Player& player,
        float maximumLaunchedTimerSeconds);

    static bool FaceTarget(Player& player, PlayerMovement& movement, const Enemy& target);
};
