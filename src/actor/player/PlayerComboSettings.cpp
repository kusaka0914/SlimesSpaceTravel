#include "actor/player/PlayerComboSettings.h"

#include "utils/YamlRead.h"

#include <algorithm>
#include <cmath>
#include <yaml-cpp/yaml.h>

namespace {
void SanitizeAttack(PlayerComboAttackSettings& attack)
{
    const auto nonnegative = [](float& value) { value = std::isfinite(value) ? std::max(0.0f, value) : 0.0f; };
    nonnegative(attack.damageMultiplier);
    nonnegative(attack.guardDamage);
    nonnegative(attack.hitDelaySeconds);
    nonnegative(attack.hitIntervalSeconds);
    nonnegative(attack.motionSeconds);
    nonnegative(attack.moveLockSeconds);
    nonnegative(attack.dodgeCancelSeconds);
    nonnegative(attack.cooldownSeconds);
    nonnegative(attack.knockBackSpeed);
    nonnegative(attack.knockBackSeconds);
    nonnegative(attack.hitStopSeconds);
    nonnegative(attack.effectScale);
    attack.animationSpeed =
        std::isfinite(attack.animationSpeed) ? std::clamp(attack.animationSpeed, 0.25f, 2.0f) : 1.0f;
    attack.hitCount = std::clamp(attack.hitCount, 1, 2);
    attack.hitIntervalSeconds = std::max(0.01f, attack.hitIntervalSeconds);
    attack.motionSeconds = std::max(0.01f, attack.motionSeconds);
    attack.hitStopSeconds = std::min(0.3f, attack.hitStopSeconds);
}

void ReadAttack(const YAML::Node& node, PlayerComboAttackSettings& attack)
{
    if (!node || !node.IsMap()) {
        return;
    }
    attack.damageMultiplier = YamlRead::ReadOr<float>(node, "damageMultiplier", attack.damageMultiplier);
    attack.guardDamage = YamlRead::ReadOr<float>(node, "guardDamage", attack.guardDamage);
    attack.hitDelaySeconds = YamlRead::ReadOr<float>(node, "hitDelaySeconds", attack.hitDelaySeconds);
    attack.hitIntervalSeconds = YamlRead::ReadOr<float>(node, "hitIntervalSeconds", attack.hitIntervalSeconds);
    attack.motionSeconds = YamlRead::ReadOr<float>(node, "motionSeconds", attack.motionSeconds);
    attack.moveLockSeconds = YamlRead::ReadOr<float>(node, "moveLockSeconds", attack.moveLockSeconds);
    attack.dodgeCancelSeconds = YamlRead::ReadOr<float>(node, "dodgeCancelSeconds", attack.dodgeCancelSeconds);
    attack.cooldownSeconds = YamlRead::ReadOr<float>(node, "cooldownSeconds", attack.cooldownSeconds);
    attack.knockBackSpeed = YamlRead::ReadOr<float>(node, "knockBackSpeed", attack.knockBackSpeed);
    attack.knockBackSeconds = YamlRead::ReadOr<float>(node, "knockBackSeconds", attack.knockBackSeconds);
    attack.hitStopSeconds = YamlRead::ReadOr<float>(node, "hitStopSeconds", attack.hitStopSeconds);
    attack.effectScale = YamlRead::ReadOr<float>(node, "effectScale", attack.effectScale);
    attack.hitCount = YamlRead::ReadOr<int>(node, "hitCount", attack.hitCount);
    attack.animationSpeed = YamlRead::ReadOr<float>(node, "animationSpeed", attack.animationSpeed);
    attack.animation = YamlRead::ReadOr<std::string>(node, "animation", attack.animation);
    attack.sound = YamlRead::ReadOr<std::string>(node, "sound", attack.sound);
}

YAML::Node WriteAttack(const PlayerComboAttackSettings& attack)
{
    YAML::Node node;
    node["damageMultiplier"] = attack.damageMultiplier;
    node["guardDamage"] = attack.guardDamage;
    node["hitDelaySeconds"] = attack.hitDelaySeconds;
    node["hitIntervalSeconds"] = attack.hitIntervalSeconds;
    node["motionSeconds"] = attack.motionSeconds;
    node["moveLockSeconds"] = attack.moveLockSeconds;
    node["dodgeCancelSeconds"] = attack.dodgeCancelSeconds;
    node["cooldownSeconds"] = attack.cooldownSeconds;
    node["knockBackSpeed"] = attack.knockBackSpeed;
    node["knockBackSeconds"] = attack.knockBackSeconds;
    node["hitStopSeconds"] = attack.hitStopSeconds;
    node["effectScale"] = attack.effectScale;
    node["hitCount"] = attack.hitCount;
    node["animationSpeed"] = attack.animationSpeed;
    node["animation"] = attack.animation;
    node["sound"] = attack.sound;
    return node;
}
} // namespace

PlayerComboSettings::PlayerComboSettings()
{
    stages[1].hitCount = 2;
    stages[1].animation = "second_attack";
    stages[3].animation = "second_attack";
    auto& finisher = stages.back();
    finisher.motionSeconds = 0.22f;
    finisher.dodgeCancelSeconds = 0.12f;
    finisher.animation = "strong_attack";
    finisher.sound = "attack_air_se";
    stages[4].damageMultiplier = 2.0f;
    stages[4].guardDamage = 10.0f;
    stages[4].hitDelaySeconds = 0.32f;
    stages[4].moveLockSeconds = 0.45f;
    stages[4].cooldownSeconds = 0.65f;
    stages[4].knockBackSpeed = 6.0f;
    stages[4].knockBackSeconds = 0.45f;
    stages[4].hitStopSeconds = 0.1f;
    stages[4].effectScale = 1.6f;
}

void PlayerComboSettings::Sanitize()
{
    for (auto& stage : stages) {
        SanitizeAttack(stage);
    }
    stages.back().hitCount = 1;
    airAttackHoverExtraSeconds =
        std::isfinite(airAttackHoverExtraSeconds) ? std::clamp(airAttackHoverExtraSeconds, 0.0f, 5.0f) : 0.5f;
    airHitPlayerLiftHeight =
        std::isfinite(airHitPlayerLiftHeight) ? std::clamp(airHitPlayerLiftHeight, 0.0f, 1.0f) : 0.15f;
    const auto clampTime = [](float value, float fallback) {
        return std::isfinite(value) ? std::clamp(value, 0.05f, 5.0f) : fallback;
    };
    continuationWindowSeconds = clampTime(continuationWindowSeconds, 1.0f);
    inputBufferSeconds = clampTime(inputBufferSeconds, 0.4f);
    assistInputBufferSeconds = clampTime(assistInputBufferSeconds, 0.55f);
}

PlayerComboSettings ReadPlayerComboSettings(const YAML::Node& node)
{
    PlayerComboSettings settings;
    if (node && node.IsMap()) {
        settings.airAttackHoverExtraSeconds = YamlRead::ReadOr<float>(
            node, "airAttackHoverExtraSeconds",
            YamlRead::ReadOr<float>(node, "airFifthAttackHoverExtraSeconds", settings.airAttackHoverExtraSeconds));
        settings.airHitPlayerLiftHeight =
            YamlRead::ReadOr<float>(node, "airHitPlayerLiftHeight", settings.airHitPlayerLiftHeight);
        settings.continuationWindowSeconds =
            YamlRead::ReadOr<float>(node, "continuationWindowSeconds", settings.continuationWindowSeconds);
        settings.inputBufferSeconds = YamlRead::ReadOr<float>(node, "inputBufferSeconds", settings.inputBufferSeconds);
        settings.assistInputBufferSeconds =
            YamlRead::ReadOr<float>(node, "assistInputBufferSeconds", settings.assistInputBufferSeconds);
        const auto stages = node["stages"];
        if (stages && stages.IsSequence()) {
            for (std::size_t index = 0; index < std::min(stages.size(), settings.stages.size()); ++index) {
                ReadAttack(stages[index], settings.stages[index]);
            }
        }
        // 旧形式で5段目の設定がない場合だけ、早期フィニッシュ設定を共通設定へ引き継ぐ。
        if (!stages || !stages.IsSequence() || stages.size() < settings.stages.size()) {
            ReadAttack(node["earlyFinisher"], settings.stages.back());
        }
    }
    settings.Sanitize();
    return settings;
}

YAML::Node WritePlayerComboSettings(const PlayerComboSettings& settings)
{
    YAML::Node node;
    node.remove("airFifthAttackHoverSeconds");
    node.remove("airFifthAttackCooldownSeconds");
    node.remove("airFifthAttackHoverExtraSeconds");
    node["airAttackHoverExtraSeconds"] = settings.airAttackHoverExtraSeconds;
    node["airHitPlayerLiftHeight"] = settings.airHitPlayerLiftHeight;
    node["continuationWindowSeconds"] = settings.continuationWindowSeconds;
    node["inputBufferSeconds"] = settings.inputBufferSeconds;
    node["assistInputBufferSeconds"] = settings.assistInputBufferSeconds;
    for (const auto& attack : settings.stages) {
        node["stages"].push_back(WriteAttack(attack));
    }
    return node;
}
