#pragma once

#include <array>
#include <string>

namespace YAML {
class Node;
}

// 地上・空中共通コンボの1入力分。hitCountは段数とは独立し、時間は秒、押し戻し速度はワールド単位/秒。
struct PlayerComboAttackSettings {
    float damageMultiplier = 1.0f;
    float guardDamage = 5.0f;
    int hitCount = 1;
    float hitDelaySeconds = 0.13f;
    float hitIntervalSeconds = 0.09f;
    // 最終命中から動作終了まで。移動ロック・ダッシュ制限・次の攻撃までの待ち時間は別々に指定する。
    float motionSeconds = 0.16f;
    float moveLockSeconds = 0.22f;
    float dodgeCancelSeconds = 0.0f;
    float cooldownSeconds = 0.0f;
    float knockBackSpeed = 3.0f;
    float knockBackSeconds = 0.3f;
    float hitStopSeconds = 0.04f;
    float effectScale = 1.0f;
    float animationSpeed = 1.0f;
    std::string animation = "attack";
    std::string sound = "attack_se";
};

// 1～4段目は弱攻撃。空中5段目は4段目を共用し、地上5段目と明示的な強攻撃は最後の設定を使う。
struct PlayerComboSettings {
    std::array<PlayerComboAttackSettings, 5> stages;
    // 空中で敵に命中した1ヒットあたりの上昇量。同時に複数体へ当てても加算しない。
    float airHitPlayerLiftHeight = 0.15f;
    // 空中各段のクールタイムに加算する落下・移動停止時間。最終命中判定から計測する。
    float airAttackHoverExtraSeconds = 0.5f;
    float continuationWindowSeconds = 1.0f;
    float inputBufferSeconds = 0.4f;
    float assistInputBufferSeconds = 0.55f;

    PlayerComboSettings();
    void Sanitize();
};

// 不足する項目は既定値を使う。保存時はエディターで編集できる項目をすべて出力する。
PlayerComboSettings ReadPlayerComboSettings(const YAML::Node& node);
YAML::Node WritePlayerComboSettings(const PlayerComboSettings& settings);
