#pragma once

struct PlayerModuleContext;

class PlayerStateMachine {
public:
    void Update(PlayerModuleContext& context, float deltaTime);

    void UpdateAlive(PlayerModuleContext& context, float deltaTime);
    void UpdateIdle(PlayerModuleContext& context, float deltaTime);
    void UpdateDodging(PlayerModuleContext& context, float deltaTime);
    void UpdateAttacking(PlayerModuleContext& context, float deltaTime);
    void UpdateCharging(PlayerModuleContext& context, float deltaTime);
    void UpdateStrongAttacking(PlayerModuleContext& context, float deltaTime);
    void UpdateKnockedBack(PlayerModuleContext& context, float deltaTime);
    void UpdateSpecialAttackCharging(PlayerModuleContext& context, float deltaTime);
    void UpdateContinuousAttacking(PlayerModuleContext& context, float deltaTime);
    void UpdateTimer(PlayerModuleContext& context, float deltaTime);
    void UpdateJewelTimer(PlayerModuleContext& context, float deltaTime);
    void UpdateComboKeepTimer(PlayerModuleContext& context, float deltaTime);
    void StartIdle(PlayerModuleContext& context);
    void StartJewelTimer(PlayerModuleContext& context);
    void StartTired(PlayerModuleContext& context, float lockTime);
    void ReduceTired(PlayerModuleContext& context);
};
