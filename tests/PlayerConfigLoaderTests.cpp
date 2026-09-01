#include "TestSupport.h"

#include "actor/player/PlayerConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

class TemporaryPlayerConfigFile {
public:
    TemporaryPlayerConfigFile()
    {
        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        mFilePath =
            std::filesystem::temp_directory_path() /
            ("space_player_config_test_" +
             std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryPlayerConfigFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mFilePath, removeError);
    }

    void Write(const std::string& yamlText) const
    {
        std::ofstream output(mFilePath);
        output << yamlText;
    }

    std::string PathText() const
    {
        return mFilePath.string();
    }

private:
    std::filesystem::path mFilePath;
};

void AirDodgeAttackParametersLoadIndependentlyFromWeakAttack()
{
    const TemporaryPlayerConfigFile configFile;
    configFile.Write(
        "players:\n"
        "  - wideAttack: 17\n"
        "    groundWeakAttackCooldownSeconds: 0.35\n"
        "    airWeakAttackCooldownSeconds: 0.65\n"
        "    airDodgeAttackDamage: 8.5\n"
        "    airDodgeHorizontalHitboxScale: 1.25\n"
        "    airDodgeVerticalHitboxScale: 2.75\n"
        "    airDodgeEnemyPushSpeed: 7.5\n"
        "    airDodgeEnemyPushDampingPerSecond: 4.5\n");

    const PlayerConfig config =
        PlayerConfigLoader::Load(configFile.PathText());

    ExpectNear(17.0f, config.wideAttack, 0.0001f, "weak attack damage");
    ExpectNear(
        0.35f,
        config.groundWeakAttackCooldownSeconds,
        0.0001f,
        "ground weak attack cooldown");
    ExpectNear(
        0.65f,
        config.airWeakAttackCooldownSeconds,
        0.0001f,
        "air weak attack cooldown");
    ExpectNear(
        8.5f,
        config.airDodgeAttackDamage,
        0.0001f,
        "air dodge attack damage");
    ExpectNear(
        1.25f,
        config.airDodgeHorizontalHitboxScale,
        0.0001f,
        "air dodge horizontal hitbox scale");
    ExpectNear(
        2.75f,
        config.airDodgeVerticalHitboxScale,
        0.0001f,
        "air dodge vertical hitbox scale");
    ExpectNear(
        7.5f,
        config.airDodgeEnemyPushSpeed,
        0.0001f,
        "air dodge enemy push speed");
    ExpectNear(
        4.5f,
        config.airDodgeEnemyPushDampingPerSecond,
        0.0001f,
        "air dodge enemy push damping");
}

void ParsedPlayerYamlProducesConfigWithoutFileAccess()
{
    const YAML::Node playerRoot = YAML::Load(
        "players:\n"
        "  - hp: 135\n"
        "    moveSpeed: 7.25\n");

    const PlayerConfig config = PlayerConfigLoader::Parse(playerRoot);

    ExpectNear(135.0f, config.hp, 0.0001f, "parsed player hp");
    ExpectNear(
        7.25f,
        config.moveSpeed,
        0.0001f,
        "parsed player move speed");
}

}

void RegisterPlayerConfigLoaderTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlayerConfigLoader.AirDodgeAttackParametersLoadIndependentlyFromWeakAttack",
        AirDodgeAttackParametersLoadIndependentlyFromWeakAttack);
    tests.emplace_back(
        "PlayerConfigLoader.ParsedPlayerYamlProducesConfigWithoutFileAccess",
        ParsedPlayerYamlProducesConfigWithoutFileAccess);
}
