#pragma once

#include "actor/player/PlayerTypes.h"

class Enemy;
class Player;
class PlayerBoatRide;
class PlayerCombat;
class PlayerGrounding;
class PlayerInput;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerRespawn;
class PlayerStatus;

class PlayerStateMachine {
public:
    void Update(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding,
                PlayerBoatRide& boatRide, PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status,
                PlayerRespawn& respawn, float deltaTime);

    PlayerActionState GetActionState() const { return mActionState; }
    void ChangeState(PlayerActionState actionState) { mActionState = actionState; }
    bool IsDodging() const { return mActionState == PlayerActionState::Dodging; }
    bool IsAttackingState() const;
    Enemy* GetAttackDirectionTarget() const { return mAttackDirectionTarget; }
    void ClearAttackDirectionTarget() { mAttackDirectionTarget = nullptr; }

private:
    void UpdateAlive(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding,
                     PlayerBoatRide& boatRide, PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status,
                     PlayerRespawn& respawn, float deltaTime);
    void UpdateIdle(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                    PlayerJewelGauge& jewelGauge, PlayerStatus& status, float deltaTime);
    void UpdateDodging(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding,
                       PlayerCombat& combat, float deltaTime);
    void UpdateAttacking(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                         PlayerStatus& status, float deltaTime);
    void UpdateStrongAttacking(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                               PlayerStatus& status, float deltaTime);
    void UpdateAirSlamAttacking(
        Player& player,
        PlayerMovement& movement,
        PlayerCombat& combat,
        float deltaTime);
    void UpdateKnockedBack(Player& player, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                           float deltaTime);
    void UpdateSpecialAttackCharging(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                     PlayerJewelGauge& jewelGauge, float deltaTime);
    void UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                                   float deltaTime);
    void UpdateTimer(PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding, PlayerCombat& combat,
                     PlayerJewelGauge& jewelGauge, PlayerStatus& status, float deltaTime);
    void UpdateCoyoteTime(const Player& player, float deltaTime);

    bool TryStartAssistStrongAttack(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                    float deltaTime);
    bool TryStartAssistBrokenEnemyAirCombo(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerCombat& combat,
        PlayerStatus& status,
        float deltaTime);
    bool TryStartAssistAirSlamAttack(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerCombat& combat);
    bool TryStartAirSlamAttack(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerCombat& combat,
        float deltaTime);
    bool ApplyIdleGravity(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerCombat& combat,
        float deltaTime);
    bool TryStartJumping(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                         float deltaTime);
    bool TryRecover(Player& player, PlayerInput& input, PlayerCombat& combat,
                    PlayerJewelGauge& jewelGauge, PlayerStatus& status);
    void UpdateIdleMovement(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                            PlayerStatus& status, bool wasInputMovementApplied, float deltaTime);
    bool TryStartSpecialAttack(PlayerInput& input, PlayerCombat& combat, PlayerJewelGauge& jewelGauge);
    bool TryReduceTired(PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status);
    bool TryStartContinuousAttack(Player& player, PlayerInput& input, PlayerCombat& combat,
                                  PlayerJewelGauge& jewelGauge);
    void UpdateSpecialAttackIfNeeded(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                     PlayerJewelGauge& jewelGauge, float deltaTime);
    bool TryUpdateContinuousAttack(Player& player, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                                   float deltaTime);
    bool TryStartDodging(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                         PlayerStatus& status);
    bool TryStartAssistAirDodgeAttack(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerCombat& combat,
        PlayerStatus& status);
    bool TryStartAttack(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                        PlayerStatus& status, float deltaTime);

    void StartIdle();
    void StartJewelTimer(PlayerJewelGauge& jewelGauge);
    void StartTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat, float lockTime);
    void ReduceTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat);

private:
    PlayerActionState mActionState = PlayerActionState::Idle;
    Enemy* mAttackDirectionTarget = nullptr;
    // 空中弱攻撃を回避でキャンセルした場合は、攻撃後の滞空硬直を
    // 回避後に持ち越さない。通常の空中回避の短い滞空は維持する。
    bool mShouldSkipAirDodgePostHover = false;
    // 空中弱攻撃を回避でキャンセルした後だけ、次の攻撃開始まで
    // 空中移動を戦闘側の移動ロックから独立して許可する。
    bool mAllowsAirMovementAfterDodge = false;
    float mCoyoteTimeRemaining = 0.0f;
    float mCoyoteTimeDuration = 0.15f;
};
