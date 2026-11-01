#include "support/TestFileSupport.h"
#include "support/TestSupport.h"

#include "actor/player/PlayerConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace {

class TemporaryPlayerConfigFile {
public:
    TemporaryPlayerConfigFile()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        mFilePath = std::filesystem::temp_directory_path() /
                    ("space_player_config_test_" + std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryPlayerConfigFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mFilePath, removeError);
    }

    void Write(const std::string& yamlText) const { WriteTestFile(mFilePath, yamlText); }

    std::string GetPathText() const { return mFilePath.string(); }

private:
    std::filesystem::path mFilePath;
};

void AirDodgeAttackParametersLoadIndependentlyFromWeakAttack()
{
    const TemporaryPlayerConfigFile configFile;
    configFile.Write("players:\n"
                     "  - wideAttack: 17\n"
                     "    groundWeakAttackCooldownSeconds: 0.35\n"
                     "    airWeakAttackCooldownSeconds: 0.65\n"
                     "    airDodgeAttackDamage: 8.5\n"
                     "    airDodgeGuardDamage: 4.5\n"
                     "    airDodgeHorizontalHitboxScale: 1.25\n"
                     "    airDodgeVerticalHitboxScale: 2.75\n"
                     "    airDodgeEnemyPushSpeed: 7.5\n"
                     "    airDodgeEnemyPushDampingPerSecond: 4.5\n"
                     "    airWeakEnemyLiftHeight: 0.6\n"
                     "    airComboDodgePlayerLiftHeight: 0.9\n"
                     "    airComboDodgeEnemyLiftHeight: 1.1\n"
                     "    airSlamEnemyDownwardSpeed: 19\n"
                     "    airSlamFullDamageHeight: 7\n"
                     "    airSlamMinimumDamageRatio: 0.25\n");

    const PlayerConfig config = PlayerConfigLoader::Load(configFile.GetPathText());

    ExpectNear(17.0f, config.wideAttack, 0.0001f, "weak attack damage");
    ExpectNear(0.35f, config.groundWeakAttackCooldownSeconds, 0.0001f, "ground weak attack cooldown");
    ExpectNear(0.65f, config.airWeakAttackCooldownSeconds, 0.0001f, "air weak attack cooldown");
    ExpectNear(8.5f, config.airDodgeAttackDamage, 0.0001f, "air dodge attack damage");
    ExpectNear(4.5f, config.airDodgeGuardDamage, 0.0001f, "air dodge guard damage");
    ExpectNear(1.25f, config.airDodgeHorizontalHitboxScale, 0.0001f, "air dodge horizontal hitbox scale");
    ExpectNear(2.75f, config.airDodgeVerticalHitboxScale, 0.0001f, "air dodge vertical hitbox scale");
    ExpectNear(7.5f, config.airDodgeEnemyPushSpeed, 0.0001f, "air dodge enemy push speed");
    ExpectNear(4.5f, config.airDodgeEnemyPushDampingPerSecond, 0.0001f, "air dodge enemy push damping");
    ExpectNear(0.6f, config.airWeakEnemyLiftHeight, 0.0001f, "air weak enemy lift height");
    ExpectNear(0.9f, config.airComboDodgePlayerLiftHeight, 0.0001f, "air combo dodge player lift height");
    ExpectNear(1.1f, config.airComboDodgeEnemyLiftHeight, 0.0001f, "air combo dodge enemy lift height");
    ExpectNear(19.0f, config.airSlamEnemyDownwardSpeed, 0.0001f, "air slam enemy downward speed");
    ExpectNear(7.0f, config.airSlamFullDamageHeight, 0.0001f, "air slam full damage height");
    ExpectNear(0.25f, config.airSlamMinimumDamageRatio, 0.0001f, "air slam minimum damage ratio");
}

void GuardDamageParametersLoadFromPlayerConfig()
{
    const YAML::Node playerRoot = YAML::Load("players:\n"
                                             "  - groundCombo1GuardDamage: 4\n"
                                             "    groundCombo2GuardDamage: 6\n"
                                             "    groundCombo3GuardDamage: 12\n"
                                             "    groundWideGuardDamage: 7\n"
                                             "    airWeakGuardDamage: 8\n"
                                             "    strongAttackGuardDamage: 21\n"
                                             "    airSlamGuardDamage: 11\n"
                                             "    continuousAttackGuardDamage: 3\n");

    const PlayerConfig config = PlayerConfigLoader::Parse(playerRoot);

    ExpectNear(4.0f, config.groundCombo1GuardDamage, 0.0001f, "combo one guard damage");
    ExpectNear(6.0f, config.groundCombo2GuardDamage, 0.0001f, "combo two guard damage");
    ExpectNear(12.0f, config.groundCombo3GuardDamage, 0.0001f, "combo three guard damage");
    ExpectNear(7.0f, config.groundWideGuardDamage, 0.0001f, "ground wide guard damage");
    ExpectNear(8.0f, config.airWeakGuardDamage, 0.0001f, "air weak guard damage");
    ExpectNear(21.0f, config.strongAttackGuardDamage, 0.0001f, "strong attack guard damage");
    ExpectNear(11.0f, config.airSlamGuardDamage, 0.0001f, "air slam guard damage");
    ExpectNear(3.0f, config.continuousAttackGuardDamage, 0.0001f, "continuous attack guard damage");
}

void HitStopParametersLoadAndClampWithoutChangingDefaults()
{
    const auto defaults = PlayerConfigLoader::Parse(YAML::Load("players: [{}]"));
    ExpectNear(0.1f, defaults.hitStop.guardBreakSeconds, 0.0001f, "guard break defaults to one tenth second");
    ExpectNear(0.4f, defaults.hitStop.airStrongAttackSeconds, 0.0001f, "air strong attack keeps its previous duration");
    const auto parsed =
        PlayerConfigLoader::Parse(YAML::Load("players: [{hitStop: {guardBreakSeconds: 0.05, airStrongAttackSeconds: 0, "
                                             "airSlamSeconds: 0.2, counterSeconds: 0.15}}]"));
    ExpectNear(0.05f, parsed.hitStop.guardBreakSeconds, 0.0001f, "guard break is configurable");
    ExpectNear(0.0f, parsed.hitStop.airStrongAttackSeconds, 0.0001f, "zero disables an event's hit stop");
    ExpectNear(0.2f, parsed.hitStop.airSlamSeconds, 0.0001f, "slam is independent from air strong attack");
    ExpectNear(0.15f, parsed.hitStop.counterSeconds, 0.0001f, "counter is configurable");
    const auto invalid =
        PlayerConfigLoader::Parse(YAML::Load("players: [{hitStop: {guardBreakSeconds: -1, counterSeconds: .nan}}]"));
    ExpectNear(0.0f, invalid.hitStop.guardBreakSeconds, 0.0001f, "negative durations are clamped to zero");
    ExpectNear(0.3f, invalid.hitStop.counterSeconds, 0.0001f, "nonfinite duration uses the default");
}

void GroundComboDefaultsAndRoundTripKeepStagesSeparateFromHits()
{
    PlayerComboSettings settings;
    ExpectEqual(2, settings.stages[1].hitCount, "only the second stage starts with two hits");
    int hits = 0;
    for (const auto& stage : settings.stages) {
        hits += stage.hitCount;
    }
    ExpectEqual(6, hits, "five default inputs produce six hit checks");
    ExpectNear(1.0f, settings.stages[0].animationSpeed, 0.0001f, "short recovery does not speed up animation");
    settings.airAttackHoverExtraSeconds = 1.25f;
    settings.airHitPlayerLiftHeight = 0.2f;
    settings.stages[1].animationSpeed = 0.9f;
    settings.stages[1].hitIntervalSeconds = 0.07f;
    settings.stages[4].damageMultiplier = 3.0f;
    const auto saved = WritePlayerComboSettings(settings);
    ExpectFalse(static_cast<bool>(saved["earlyFinisher"]), "saving writes only one finisher setting");
    const auto restored = ReadPlayerComboSettings(saved);
    ExpectNear(1.25f, restored.airAttackHoverExtraSeconds, 0.0001f, "air attack hover extension survives save and load");
    ExpectNear(0.2f, restored.airHitPlayerLiftHeight, 0.0001f, "air hit player lift survives save and load");
    ExpectNear(0.9f, restored.stages[1].animationSpeed, 0.0001f, "animation speed survives save and load");
    ExpectNear(0.07f, restored.stages[1].hitIntervalSeconds, 0.0001f, "multi hit interval survives save and load");
    ExpectNear(3.0f, restored.stages[4].damageMultiplier, 0.0001f, "finisher damage survives save and load");
    const auto parsed =
        PlayerConfigLoader::Parse(YAML::Load("players: [{groundCombo: {stages: [{hitDelaySeconds: 0.11}, {hitCount: "
                                             "2}], earlyFinisher: {knockBackSpeed: 4}}}]"));
    ExpectNear(0.11f, parsed.combo.stages[0].hitDelaySeconds, 0.0001f,
               "ground timing is read separately from air timing");
    ExpectNear(4.0f, parsed.combo.stages.back().knockBackSpeed, 0.0001f,
               "legacy early finisher migrates to the shared finisher");
    const auto common = PlayerConfigLoader::Parse(YAML::Load(
        "players: [{combo: {stages: [{damageMultiplier: 2.5}]}, groundCombo: {stages: [{damageMultiplier: 9}]}}]"));
    ExpectNear(2.5f, common.combo.stages[0].damageMultiplier, 0.0001f,
               "common combo settings take precedence over the legacy ground only key");
    settings.stages[4].hitCount = 2;
    settings.stages[0].hitIntervalSeconds = -1.0f;
    settings.Sanitize();
    ExpectEqual(1, settings.stages[4].hitCount, "a finisher remains a single hit");
    ExpectTrue(settings.stages[0].hitIntervalSeconds > 0.0f, "invalid hit interval cannot cause a zero time loop");
}

void ParsedPlayerYamlProducesConfigWithoutFileAccess()
{
    const YAML::Node playerRoot = YAML::Load("players:\n"
                                             "  - hp: 135\n"
                                             "    moveSpeed: 7.25\n");

    const PlayerConfig config = PlayerConfigLoader::Parse(playerRoot);

    ExpectNear(135.0f, config.hp, 0.0001f, "parsed player hp");
    ExpectNear(7.25f, config.moveSpeed, 0.0001f, "parsed player move speed");
}

} // namespace

void RegisterPlayerConfigLoaderTests(TestRegistry& tests)
{
    tests.emplace_back("PlayerConfigLoader.HitStopParameters", HitStopParametersLoadAndClampWithoutChangingDefaults);
    tests.emplace_back("PlayerConfigLoader.GroundComboRoundTrip",
                       GroundComboDefaultsAndRoundTripKeepStagesSeparateFromHits);
    tests.emplace_back("PlayerConfigLoader.AirDodgeAttackParametersLoadIndependentlyFromWeakAttack",
                       AirDodgeAttackParametersLoadIndependentlyFromWeakAttack);
    tests.emplace_back("PlayerConfigLoader.ParsedPlayerYamlProducesConfigWithoutFileAccess",
                       ParsedPlayerYamlProducesConfigWithoutFileAccess);
    tests.emplace_back("PlayerConfigLoader.GuardDamageParametersLoadFromPlayerConfig",
                       GuardDamageParametersLoadFromPlayerConfig);
}
