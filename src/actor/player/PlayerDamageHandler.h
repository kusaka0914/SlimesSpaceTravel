#pragma once

#include <glm/vec3.hpp>

class Enemy;
class Player;
class PlayerCombat;
class PlayerInput;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerStateMachine;
class PlayerStatus;

// 被ダメージとジャスト回避を処理し、HP・無敵・疲労・ノックバック・攻撃中断を反映する。
class PlayerDamageHandler {
public:
    // 敵がカウンター可能で回避中ならガード削りとジュエル回復を行う。それ以外は通常被弾を処理する。
    // enemyがnullptrなら何もしない。通常被弾は無敵中なら無視し、空中ではダメージを2倍にする。
    static void Apply(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerStateMachine& stateMachine,
                      PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status, Enemy* enemy,
                      float deltaTime);

    // 配置物からの被弾を処理する。無敵中は無視し、成立時はノックバックと攻撃中断を反映する。
    // damageSourcePositionはワールド座標。空中ではダメージを2倍にする。
    static void ApplyFromActor(Player& player, PlayerInput& input, PlayerMovement& movement,
                               PlayerStateMachine& stateMachine, PlayerCombat& combat, PlayerStatus& status,
                               const glm::vec3& damageSourcePosition, float damage);
};
