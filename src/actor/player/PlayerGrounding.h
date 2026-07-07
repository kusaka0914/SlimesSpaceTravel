#pragma once

class Player;
class PlayerCombat;
class PlayerMovement;

class PlayerGrounding {
public:
    void OnLanded(Player& player, PlayerMovement& movement, PlayerCombat& combat);
    void OnUpVecUpdateFailed(Player& player);
    void OnCastSucceeded();
    void SnapToGround(Player& player, float upOffset, float downLength);

    void UpdateRayCastTimer(float deltaTime);
    void ResetRayCastTimer() { mRayCastTimer = 0.5f; }
    void SetRayCastTimer(float rayCastTimer) { mRayCastTimer = rayCastTimer; }
    float GetRayCastTimer() const { return mRayCastTimer; }

private:
    float mRayCastTimer = 0.5f;
};
