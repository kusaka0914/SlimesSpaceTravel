#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// 行動の種類・選択重み・パラメーター。parametersのキーと単位は各行動の実装に従う。
struct EnemyBehaviorActionConfig {
    std::string type;
    float weight = 1.0f;
    std::unordered_map<std::string, float> parameters;

    float GetParameter(const std::string& name, float fallback) const
    {
        const auto found = parameters.find(name);
        return found != parameters.end() ? found->second : fallback;
    }
};

// 行動プロファイル名と実行候補の一覧。行動の生成・選択はEnemyBehaviorControllerが担当する。
struct EnemyBehaviorConfig {
    std::string profileName = "legacyMelee";
    std::vector<EnemyBehaviorActionConfig> actions;
};

// ボスの攻撃前接近・攻撃後退避の設定。確率は%、時間は秒、距離はワールド単位。
struct EnemyBossManeuverConfig {
    float preAttackApproachProbabilityPercent = 0.0f;
    float preAttackApproachSpeed = 12.0f;
    float preAttackApproachStopDistance = 2.5f;
    float postAttackRetreatProbabilityPercent = 0.0f;
    float postAttackRetreatDelaySeconds = 1.0f;
    float postAttackRetreatSpeed = 12.0f;
    float postAttackRetreatDistance = 4.0f;
    float postRetreatRecoverySeconds = 2.0f;
    float postRetreatFollowupApproachProbabilityPercent = 0.0f;
};

// 読み込み後にEnemyへ適用する設定。時間は秒、距離はワールド単位。行動固有の単位は各実装に従う。
struct EnemyConfig {
    bool isBoss = false;
    bool isBossEncounter = false;
    bool isNormalHitKnockBackEnabled = true;

    float hp = 80.0f;
    float scale = 0.25f;
    float moveSpeed = 1.0f;
    float attack = 5.0f;
    float radius = 0.75f;

    int guardSegmentCount = 1;
    float guardValuePerSegment = 20.0f;

    std::string modelPath;

    float defaultStandByAttackTimer = 0.0f;
    float defaultAttackMotionTimer = 0.0f;
    float attackSpeed = 0.0f;
    float attackPreparationRange = 0.0f;

    float knockBackSpeed = 0.0f;
    float defaultLaunchedTimer = 0.0f;
    float launchHeight = 1.2755f;
    float detectionRange = 0.0f;

    EnemyBehaviorConfig behavior;
    EnemyBossManeuverConfig bossManeuver;
};
