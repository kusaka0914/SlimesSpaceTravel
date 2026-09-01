#include "TestSupport.h"

#include "actor/enemy/EnemyConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig()
{
    const YAML::Node enemyRoot = YAML::Load(
        "enemies:\n"
        "  - type: common\n"
        "    detectionRange: 14\n"
        "  - type: guard\n"
        "    hp: 240\n"
        "    speed: 3.5\n");

    const EnemyConfig config =
        EnemyConfigLoader::Parse(enemyRoot, "guard");

    ExpectNear(
        14.0f,
        config.detectionRange,
        0.0001f,
        "common detection range");
    ExpectNear(240.0f, config.hp, 0.0001f, "selected enemy hp");
    ExpectNear(
        3.5f,
        config.moveSpeed,
        0.0001f,
        "selected enemy move speed");
}

}

void RegisterEnemyConfigLoaderTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "EnemyConfigLoader.ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig",
        ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig);
}
