#pragma once

class Player;
class PlayerCombat;
class PlayerMovement;
class PhysicsSystem;

class PlayerGrounding {
public:
    explicit PlayerGrounding(PhysicsSystem& physicsSystem);

    void OnLanded(Player& player, PlayerMovement& movement, PlayerCombat& combat);
    void OnUpVecUpdateFailed(Player& player);
    void OnCastSucceeded();
    void SnapToGround(Player& player, float upOffset, float downLength);

    void UpdateRayCastTimer(float deltaTime);
    void ResetRayCastTimer() { mRayCastTimer = 0.5f; }
    void SetRayCastTimer(float rayCastTimer) { mRayCastTimer = rayCastTimer; }
    float GetRayCastTimer() const { return mRayCastTimer; }

private:
    PhysicsSystem& mPhysicsSystem;
    float mRayCastTimer = 0.5f;
};
