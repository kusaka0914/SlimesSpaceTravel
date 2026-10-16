#pragma once

class Player;

// 歩き始め・着地・特殊攻撃の溜めに応じてパーティクルを発生させ、発生間隔を管理する。
class PlayerParticleEffectController {
public:
    // 歩き始めたフレームだけ発生させる。継続歩行中には繰り返さない。
    void UpdateWalking(Player& player, bool isWalking);
    void UpdateSpecialCharging(Player& player, bool isCharging, float deltaTime);

    // landingSpeedは着地直前の下向き速度（ワールド距離/秒）。1未満なら発生させない。
    void EmitLanding(Player& player, float landingSpeed);

    // 発生履歴と溜めのタイマーを戻す。既に生成したパーティクルは消去しない。
    void Reset();

private:
    void EmitWalkStart(Player& player);
    void EmitSpecialCharge(Player& player);

private:
    bool mWasWalking = false;
    bool mWasSpecialCharging = false;
    float mSpecialChargeEmissionTimerSeconds = 0.0f;
};
