#pragma once

#include <cstdint>
#include <string>
#include <vector>

// 編集・保存用の攻撃設定。確率は%、角度は度、時間は秒、距離はワールド単位。
struct EnemyAttackPresetDefinition {
    std::string type = "meleeAttack";
    float selectionProbabilityPercent = 100.0f;

    int chargeCount = 3;
    float repeatDelaySeconds = 1.0f;
    float range = 5.0f;
    float angleDegrees = 120.0f;
    float windUpDurationSeconds = 2.0f;
    float attackDurationSeconds = 0.5f;
};

// IDと表示名を持つ編集用の敵設定。実行時のEnemyConfigへの変換はEnemyConfigLoaderが担当する。
struct EnemyPresetDefinition {
    std::string id;
    std::string displayName;
    std::string behaviorProfile = "legacyMelee";
    bool isBoss = false;
    bool isBossEncounter = false;
    bool isNormalHitKnockBackEnabled = true;

    float hp = 80.0f;
    std::string modelPath = "enemy.obj";
    float scale = 0.25f;
    float moveSpeed = 1.0f;
    float attack = 5.0f;
    int guardSegmentCount = 1;
    float radius = 0.75f;
    float attackIntervalSeconds = 0.0f;
    float attackMotionDurationSeconds = 0.0f;
    float attackSpeed = 0.0f;
    float attackPreparationRange = 2.25f;

    float preAttackApproachProbabilityPercent = 0.0f;
    float preAttackApproachSpeed = 12.0f;
    float preAttackApproachStopDistance = 2.5f;
    float postAttackRetreatProbabilityPercent = 0.0f;
    float postAttackRetreatDelaySeconds = 1.0f;
    float postAttackRetreatSpeed = 12.0f;
    float postAttackRetreatDistance = 4.0f;
    float postRetreatRecoverySeconds = 2.0f;
    float postRetreatFollowupApproachProbabilityPercent = 0.0f;
    std::vector<EnemyAttackPresetDefinition> attacks;
};

// 編集用の敵プリセットをYAMLから読み込み、検証・攻撃確率の正規化を行って保存する。
class EnemyPresetRepository {
public:
    // common以外のプリセットを読み込み、成功時だけoutPresetsを置き換える。
    // 失敗時はfalseを返してoutErrorMessageへ理由を設定し、元のoutPresetsを保持する。
    static bool Load(const std::string& filePath, std::vector<EnemyPresetDefinition>& outPresets,
                     std::string& outErrorMessage);

    // 既存YAMLへ保存する。originalPresetIdが空なら追加、指定時はそのIDを置換し、攻撃確率も正規化する。
    // 成功時だけファイルを置き換えて更新番号を進める。失敗時は既存ファイルを保ち、falseと理由を返す。
    static bool Save(const std::string& filePath, const std::string& originalPresetId,
                     const EnemyPresetDefinition& preset, std::string& outErrorMessage);

    // sourceIdに_copyと連番を付け、指定一覧と重複しないIDを返す。空のsourceIdはenemyとして扱う。
    static std::string CreateUniqueId(const std::string& sourceId, const std::vector<EnemyPresetDefinition>& presets);

    // 指定タイプの初期設定を返す。編集対象外のタイプはmeleeAttackへ置き換える。
    static EnemyAttackPresetDefinition CreateDefaultAttack(const std::string& attackType);

    // 負の確率を0にし、合計100%へ正規化する。合計がほぼ0なら均等配分し、空配列は変更しない。
    static void NormalizeAttackProbabilities(std::vector<EnemyAttackPresetDefinition>& attacks);

    // このプロセス内でSaveが成功するたびに増える更新番号。外部からのファイル変更は検出しない。
    static std::uint64_t GetRevision();
};
