#pragma once

class Player;
class ParticleSystem;

// 歩き始め・着地・特殊攻撃の溜め・叩きつけの高度に応じて発生間隔を管理する。
// PlayerとParticleSystemは呼び出し中だけ借用し、ActorやGameへの参照を保持しない。
class PlayerParticleEffectController {
public:
    // 歩き始めたフレームだけ発生させる。継続歩行中には繰り返さない。
    void UpdateWalking(Player& player, ParticleSystem& particles, bool isWalking);
    // 開始時は小さく、ジュエル消費ごとに拡大する。2個目の消費時は一度だけ大きく発光する。
    void UpdateSpecialCharging(Player& player, ParticleSystem& particles, bool isCharging, float deltaTime);

    // 通常ジャンプを超える高度だけ溜めオーラを表示し、最大威力到達時は一度だけ強調する。
    // 発動後は開始高度の表示を着地まで維持する。無効時は履歴を戻し、ダメージや行動状態は変更しない。
    void UpdateAirSlamReadiness(Player& player, ParticleSystem& particles, bool isEnabled, float deltaTime);

    // landingSpeedは着地直前の下向き速度（ワールド距離/秒）。1未満なら発生させない。
    void EmitLanding(Player& player, ParticleSystem& particles, float landingSpeed);

    // 発生履歴と溜めのタイマーを戻す。既に生成したパーティクルは消去しない。
    void Reset();

private:
    void EmitWalkStart(Player& player, ParticleSystem& particles);
    void EmitChargeAura(Player& player, ParticleSystem& particles, float scale);

private:
    bool mWasWalking = false;
    int mPreviousSpecialChargeStage = -1;
    float mSpecialChargeEmissionTimerSeconds = 0.0f;
    float mAirSlamEmissionTimerSeconds = 0.0f;
    bool mWasAirSlamAtFullPower = false;
};
