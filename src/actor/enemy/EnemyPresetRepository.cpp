#include "actor/enemy/EnemyPresetRepository.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <optional>
#include <unordered_set>

#include <yaml-cpp/yaml.h>

namespace {
constexpr const char* commonEnemyConfigId = "common";
std::uint64_t enemyPresetRevision = 0;

template <typename ValueType>
ValueType ReadValue(
    const YAML::Node& node,
    const char* key,
    const ValueType& fallback)
{
    if (!node[key]) {
        return fallback;
    }

    try {
        return node[key].as<ValueType>();
    } catch (const YAML::Exception&) {
        return fallback;
    }
}

bool IsValidPresetId(const std::string& presetId)
{
    if (presetId.empty() || presetId == commonEnemyConfigId) {
        return false;
    }

    return std::all_of(
        presetId.begin(),
        presetId.end(),
        [](unsigned char character) {
            return std::isalnum(character) ||
                   character == '_' ||
                   character == '-';
        });
}

bool IsEditableAttackType(const std::string& attackType)
{
    return attackType == "meleeAttack" ||
           attackType == "tripleChargeAttack" ||
           attackType == "fanAttack" ||
           attackType == "radialAttack";
}

EnemyAttackPresetDefinition ReadAttack(const YAML::Node& node)
{
    const std::string attackType = ReadValue<std::string>(
        node,
        "type",
        "meleeAttack");
    EnemyAttackPresetDefinition attack =
        EnemyPresetRepository::CreateDefaultAttack(attackType);
    attack.selectionProbabilityPercent = ReadValue<float>(
        node,
        "weight",
        attack.selectionProbabilityPercent);
    attack.chargeCount = ReadValue<int>(
        node,
        "chargeCount",
        attack.chargeCount);
    attack.repeatDelaySeconds = ReadValue<float>(
        node,
        "repeatDelay",
        attack.repeatDelaySeconds);
    attack.range = ReadValue<float>(node, "range", attack.range);
    attack.angleDegrees = ReadValue<float>(
        node,
        "angleDegrees",
        attack.angleDegrees);
    attack.windUpDurationSeconds = ReadValue<float>(
        node,
        "windUpDuration",
        attack.windUpDurationSeconds);
    attack.attackDurationSeconds = ReadValue<float>(
        node,
        "attackDuration",
        attack.attackDurationSeconds);
    return attack;
}

std::vector<EnemyAttackPresetDefinition> ReadAttacks(
    const YAML::Node& presetNode,
    const YAML::Node& root)
{
    YAML::Node actions;
    if (presetNode.IsMap()) {
        const YAML::Node configuredAttacks = presetNode["attacks"];
        if (configuredAttacks && configuredAttacks.IsSequence()) {
            actions = configuredAttacks;
        }
    }

    if (!actions || !actions.IsSequence()) {
        const std::string behaviorProfile = ReadValue<std::string>(
            presetNode,
            "behaviorProfile",
            "legacyMelee");
        const YAML::Node profiles = root["behaviorProfiles"];
        const YAML::Node profile =
            profiles && profiles.IsMap()
                ? profiles[behaviorProfile]
                : YAML::Node();
        actions = profile && profile.IsMap()
            ? profile["actions"]
            : YAML::Node();
    }

    std::vector<EnemyAttackPresetDefinition> attacks;
    if (!actions || !actions.IsSequence()) {
        attacks.push_back(
            EnemyPresetRepository::CreateDefaultAttack("meleeAttack"));
        return attacks;
    }

    for (std::size_t actionIndex = 0;
         actionIndex < actions.size();
         ++actionIndex) {
        const YAML::Node actionNode = actions[actionIndex];
        if (!actionNode || !actionNode.IsMap()) {
            continue;
        }

        const std::string attackType = ReadValue<std::string>(
            actionNode,
            "type",
            "");
        if (!IsEditableAttackType(attackType)) {
            continue;
        }
        attacks.push_back(ReadAttack(actionNode));
    }

    if (attacks.empty()) {
        attacks.push_back(
            EnemyPresetRepository::CreateDefaultAttack("meleeAttack"));
    }
    EnemyPresetRepository::NormalizeAttackProbabilities(attacks);
    return attacks;
}

EnemyPresetDefinition ReadPreset(
    const YAML::Node& node,
    const YAML::Node& root)
{
    EnemyPresetDefinition preset;
    preset.id = ReadValue<std::string>(node, "type", "");
    preset.displayName = ReadValue<std::string>(
        node,
        "displayName",
        preset.id);
    preset.behaviorProfile = ReadValue<std::string>(
        node,
        "behaviorProfile",
        preset.behaviorProfile);
    preset.isBoss = ReadValue<bool>(
        node,
        "isBoss",
        preset.id == "boss");
    preset.hp = ReadValue<float>(node, "hp", preset.hp);
    preset.modelPath = ReadValue<std::string>(
        node,
        "modelPath",
        preset.modelPath);
    preset.scale = ReadValue<float>(node, "scale", preset.scale);
    preset.moveSpeed = ReadValue<float>(node, "speed", preset.moveSpeed);
    preset.attack = ReadValue<float>(node, "attack", preset.attack);
    preset.breakCountMax = ReadValue<int>(
        node,
        "breakCountMax",
        preset.breakCountMax);
    preset.radius = ReadValue<float>(node, "radius", preset.radius);
    preset.attackIntervalSeconds = ReadValue<float>(
        node,
        "defaultStandByAttackTimer",
        preset.attackIntervalSeconds);
    preset.attackMotionDurationSeconds = ReadValue<float>(
        node,
        "defaultAttackMotionTimer",
        preset.attackMotionDurationSeconds);
    preset.attackSpeed = ReadValue<float>(
        node,
        "attackSpeed",
        preset.attackSpeed);
    preset.attackPreparationRange = ReadValue<float>(
        node,
        "attackPreparationRange",
        preset.radius + 1.5f);
    preset.preAttackApproachProbabilityPercent = ReadValue<float>(
        node,
        "preAttackApproachProbabilityPercent",
        preset.preAttackApproachProbabilityPercent);
    preset.preAttackApproachSpeed = ReadValue<float>(
        node,
        "preAttackApproachSpeed",
        preset.preAttackApproachSpeed);
    preset.preAttackApproachStopDistance = ReadValue<float>(
        node,
        "preAttackApproachStopDistance",
        preset.preAttackApproachStopDistance);
    preset.postAttackRetreatProbabilityPercent = ReadValue<float>(
        node,
        "postAttackRetreatProbabilityPercent",
        preset.postAttackRetreatProbabilityPercent);
    preset.postAttackRetreatDelaySeconds = ReadValue<float>(
        node,
        "postAttackRetreatDelaySeconds",
        preset.postAttackRetreatDelaySeconds);
    preset.postAttackRetreatSpeed = ReadValue<float>(
        node,
        "postAttackRetreatSpeed",
        preset.postAttackRetreatSpeed);
    preset.postAttackRetreatDistance = ReadValue<float>(
        node,
        "postAttackRetreatDistance",
        preset.postAttackRetreatDistance);
    preset.postRetreatRecoverySeconds = ReadValue<float>(
        node,
        "postRetreatRecoverySeconds",
        preset.postRetreatRecoverySeconds);
    preset.postRetreatFollowupApproachProbabilityPercent =
        ReadValue<float>(
            node,
            "postRetreatFollowupApproachProbabilityPercent",
            preset.postRetreatFollowupApproachProbabilityPercent);
    preset.attacks = ReadAttacks(node, root);
    return preset;
}

void WriteAttack(
    YAML::Node& node,
    const EnemyAttackPresetDefinition& attack)
{
    node["type"] = attack.type;
    node["weight"] = attack.selectionProbabilityPercent;

    if (attack.type == "tripleChargeAttack") {
        node["chargeCount"] = attack.chargeCount;
        node["repeatDelay"] = attack.repeatDelaySeconds;
        return;
    }

    if (attack.type == "fanAttack") {
        node["range"] = attack.range;
        node["angleDegrees"] = attack.angleDegrees;
        node["windUpDuration"] = attack.windUpDurationSeconds;
        node["attackDuration"] = attack.attackDurationSeconds;
        return;
    }

    if (attack.type == "radialAttack") {
        node["range"] = attack.range;
        node["windUpDuration"] = attack.windUpDurationSeconds;
        node["attackDuration"] = attack.attackDurationSeconds;
    }
}

void WritePreset(
    YAML::Node& node,
    const EnemyPresetDefinition& preset)
{
    node["type"] = preset.id;
    node["displayName"] = preset.displayName;
    node["behaviorProfile"] = preset.behaviorProfile;
    node["isBoss"] = preset.isBoss;
    node["hp"] = preset.hp;
    node["modelPath"] = preset.modelPath;
    node["scale"] = preset.scale;
    node["speed"] = preset.moveSpeed;
    node["attack"] = preset.attack;
    node["breakCountMax"] = preset.breakCountMax;
    node["radius"] = preset.radius;
    node["defaultStandByAttackTimer"] =
        preset.attackIntervalSeconds;
    node["defaultAttackMotionTimer"] =
        preset.attackMotionDurationSeconds;
    node["attackSpeed"] = preset.attackSpeed;
    node["attackPreparationRange"] = preset.attackPreparationRange;
    node["preAttackApproachProbabilityPercent"] =
        preset.preAttackApproachProbabilityPercent;
    node["preAttackApproachSpeed"] = preset.preAttackApproachSpeed;
    node["preAttackApproachStopDistance"] =
        preset.preAttackApproachStopDistance;
    node["postAttackRetreatProbabilityPercent"] =
        preset.postAttackRetreatProbabilityPercent;
    node["postAttackRetreatDelaySeconds"] =
        preset.postAttackRetreatDelaySeconds;
    node["postAttackRetreatSpeed"] = preset.postAttackRetreatSpeed;
    node["postAttackRetreatDistance"] = preset.postAttackRetreatDistance;
    node["postRetreatRecoverySeconds"] =
        preset.postRetreatRecoverySeconds;
    node["postRetreatFollowupApproachProbabilityPercent"] =
        preset.postRetreatFollowupApproachProbabilityPercent;

    YAML::Node attacksNode(YAML::NodeType::Sequence);
    for (const EnemyAttackPresetDefinition& attack : preset.attacks) {
        YAML::Node attackNode(YAML::NodeType::Map);
        WriteAttack(attackNode, attack);
        attacksNode.push_back(attackNode);
    }
    node["attacks"] = attacksNode;
}

bool SaveYaml(
    const std::string& filePath,
    const YAML::Node& root,
    std::string& outErrorMessage)
{
    std::ofstream output(filePath);
    if (!output.is_open()) {
        outErrorMessage = "敵プリセットファイルを開けませんでした。";
        return false;
    }

    output << root;
    if (!output.good()) {
        outErrorMessage = "敵プリセットファイルの書き込みに失敗しました。";
        return false;
    }
    return true;
}
} // namespace

bool EnemyPresetRepository::Load(
    const std::string& filePath,
    std::vector<EnemyPresetDefinition>& outPresets,
    std::string& outErrorMessage)
{
    outErrorMessage.clear();

    try {
        const YAML::Node root = YAML::LoadFile(filePath);
        const YAML::Node enemies = root["enemies"];
        if (!enemies || !enemies.IsSequence()) {
            outErrorMessage =
                "enemies.yaml に enemies 一覧がありません。";
            return false;
        }

        std::vector<EnemyPresetDefinition> loadedPresets;
        loadedPresets.reserve(enemies.size());
        for (std::size_t enemyIndex = 0;
             enemyIndex < enemies.size();
             ++enemyIndex) {
            const YAML::Node enemyNode = enemies[enemyIndex];
            if (!enemyNode || !enemyNode.IsMap()) {
                continue;
            }

            const std::string presetId = ReadValue<std::string>(
                enemyNode,
                "type",
                "");
            if (presetId.empty() ||
                presetId == commonEnemyConfigId) {
                continue;
            }

            const EnemyPresetDefinition preset =
                ReadPreset(enemyNode, root);
            loadedPresets.push_back(preset);
        }

        outPresets = std::move(loadedPresets);
        return true;
    } catch (const std::exception& exception) {
        outErrorMessage =
            "敵プリセットの読み込みに失敗しました: " +
            std::string(exception.what());
        return false;
    } catch (...) {
        outErrorMessage =
            "敵プリセットの読み込み中に不明なエラーが発生しました。";
        return false;
    }
}

bool EnemyPresetRepository::Save(
    const std::string& filePath,
    const std::string& originalPresetId,
    const EnemyPresetDefinition& preset,
    std::string& outErrorMessage)
{
    outErrorMessage.clear();
    if (!IsValidPresetId(preset.id)) {
        outErrorMessage =
            "IDには半角英数字、_、-だけを使用してください。";
        return false;
    }
    if (preset.attacks.empty()) {
        outErrorMessage =
            "攻撃構成には1つ以上の攻撃が必要です。";
        return false;
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& exception) {
        outErrorMessage = exception.what();
        return false;
    }

    YAML::Node enemies = root["enemies"];
    if (!enemies || !enemies.IsSequence()) {
        outErrorMessage = "enemies.yaml に enemies 一覧がありません。";
        return false;
    }

    std::optional<std::size_t> sourceIndex;
    for (std::size_t index = 0; index < enemies.size(); ++index) {
        const std::string currentId = ReadValue<std::string>(
            enemies[index],
            "type",
            "");
        if (currentId == preset.id &&
            currentId != originalPresetId) {
            outErrorMessage = "同じIDの敵プリセットが既にあります。";
            return false;
        }
        if (!originalPresetId.empty() &&
            currentId == originalPresetId) {
            sourceIndex = index;
        }
    }

    if (!originalPresetId.empty() && !sourceIndex) {
        outErrorMessage = "編集元の敵プリセットが見つかりません。";
        return false;
    }

    if (sourceIndex) {
        YAML::Node presetNode = enemies[*sourceIndex];
        EnemyPresetDefinition normalizedPreset = preset;
        NormalizeAttackProbabilities(normalizedPreset.attacks);
        WritePreset(presetNode, normalizedPreset);
        enemies[*sourceIndex] = presetNode;
    } else {
        YAML::Node presetNode(YAML::NodeType::Map);
        EnemyPresetDefinition normalizedPreset = preset;
        NormalizeAttackProbabilities(normalizedPreset.attacks);
        WritePreset(presetNode, normalizedPreset);
        enemies.push_back(presetNode);
    }

    root["enemies"] = enemies;
    if (!SaveYaml(filePath, root, outErrorMessage)) {
        return false;
    }

    ++enemyPresetRevision;
    return true;
}

EnemyAttackPresetDefinition EnemyPresetRepository::CreateDefaultAttack(
    const std::string& attackType)
{
    EnemyAttackPresetDefinition attack;
    attack.type = IsEditableAttackType(attackType)
        ? attackType
        : "meleeAttack";

    if (attack.type == "fanAttack") {
        attack.range = 6.0f;
    }
    return attack;
}

void EnemyPresetRepository::NormalizeAttackProbabilities(
    std::vector<EnemyAttackPresetDefinition>& attacks)
{
    if (attacks.empty()) {
        return;
    }

    float probabilityTotal = 0.0f;
    for (EnemyAttackPresetDefinition& attack : attacks) {
        attack.selectionProbabilityPercent = std::max(
            0.0f,
            attack.selectionProbabilityPercent);
        probabilityTotal += attack.selectionProbabilityPercent;
    }

    if (probabilityTotal <= 0.0001f) {
        const float equalProbability =
            100.0f / static_cast<float>(attacks.size());
        for (EnemyAttackPresetDefinition& attack : attacks) {
            attack.selectionProbabilityPercent = equalProbability;
        }
        return;
    }

    const float normalizationScale = 100.0f / probabilityTotal;
    for (EnemyAttackPresetDefinition& attack : attacks) {
        attack.selectionProbabilityPercent *= normalizationScale;
    }
}

std::string EnemyPresetRepository::CreateUniqueId(
    const std::string& sourceId,
    const std::vector<EnemyPresetDefinition>& presets)
{
    std::unordered_set<std::string> existingIds;
    for (const EnemyPresetDefinition& preset : presets) {
        existingIds.insert(preset.id);
    }

    const std::string baseId =
        sourceId.empty() ? "enemy" : sourceId;
    for (int copyNumber = 1;; ++copyNumber) {
        const std::string candidateId =
            baseId + "_copy" + std::to_string(copyNumber);
        if (!existingIds.contains(candidateId)) {
            return candidateId;
        }
    }
}

std::uint64_t EnemyPresetRepository::GetRevision()
{
    return enemyPresetRevision;
}
