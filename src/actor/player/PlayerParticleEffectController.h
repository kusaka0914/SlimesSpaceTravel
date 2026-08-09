#pragma once

class Player;

class PlayerParticleEffectController {
public:
    void UpdateWalking(Player& player, bool isWalking);
    void UpdateSpecialCharging(Player& player, bool isCharging, float deltaTime);
    void EmitLanding(Player& player, float landingSpeed);
    void Reset();

private:
    void EmitWalkStart(Player& player);
    void EmitSpecialCharge(Player& player);

private:
    bool mWasWalking = false;
    bool mWasSpecialCharging = false;
    float mSpecialChargeEmissionTimerSeconds = 0.0f;
};
