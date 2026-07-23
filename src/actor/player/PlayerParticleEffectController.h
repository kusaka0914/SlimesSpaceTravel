#pragma once

class Player;

class PlayerParticleEffectController {
public:
    void UpdateWalking(Player& player, bool isWalking);
    void EmitLanding(Player& player, float landingSpeed);
    void Reset();

private:
    void EmitWalkStart(Player& player);

private:
    bool mWasWalking = false;
};
