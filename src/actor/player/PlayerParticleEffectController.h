#pragma once

class Player;
class ParticleSystem;

// 歩き始め・着地・特殊攻撃の溜めに応じて発生間隔を管理する。
// PlayerとParticleSystemは呼び出し中だけ借用し、ActorやGameへの参照を保持しない。
class PlayerParticleEffectController {
public:
    // 歩き始めたフレームだけ発生させる。継続歩行中には繰り返さない。
    void UpdateWalking(Player& player, ParticleSystem& particles, bool isWalking);
    void UpdateSpecialCharging(Player& player, ParticleSystem& particles, bool isCharging, float deltaTime);

    // landingSpeedは着地直前の下向き速度（ワールド距離/秒）。1未満なら発生させない。
    void EmitLanding(Player& player, ParticleSystem& particles, float landingSpeed);

    // 発生履歴と溜めのタイマーを戻す。既に生成したパーティクルは消去しない。
    void Reset();

private:
    void EmitWalkStart(Player& player, ParticleSystem& particles);
    void EmitSpecialCharge(Player& player, ParticleSystem& particles);

private:
    bool mWasWalking = false;
    bool mWasSpecialCharging = false;
    float mSpecialChargeEmissionTimerSeconds = 0.0f;
};
