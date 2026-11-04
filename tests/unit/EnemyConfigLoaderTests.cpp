#include "support/TestSupport.h"

#include "actor/enemy/EnemyConfigLoader.h"

#include <yaml-cpp/yaml.h>

namespace {

void ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig()
{
    const YAML::Node enemyRoot = YAML::Load("enemies:\n"
                                            "  - type: common\n"
                                            "    detectionRange: 14\n"
                                            "    attackAnimationSpeed: 0.75\n"
                                            "    attackHitDelaySeconds: 0.4\n"
                                            "  - type: guard\n"
                                            "    hp: 240\n"
                                            "    speed: 3.5\n");

    const EnemyConfig config = EnemyConfigLoader::Parse(enemyRoot, "guard");

    ExpectNear(14.0f, config.detectionRange, 0.0001f, "common detection range");
    ExpectNear(0.75f, config.attackAnimationSpeed, 0.0001f, "common attack animation speed");
    ExpectNear(0.4f, config.attackHitDelaySeconds, 0.0001f, "common delay from animation start to hit");
    ExpectNear(240.0f, config.hp, 0.0001f, "selected enemy hp");
    ExpectNear(3.5f, config.moveSpeed, 0.0001f, "selected enemy move speed");
}

void LegacyBreakCountMaxConfiguresGuardSegments()
{
    const YAML::Node enemyRoot = YAML::Load("enemies:\n"
                                            "  - type: common\n"
                                            "    guardValuePerSegment: 25\n"
                                            "  - type: guard\n"
                                            "    breakCountMax: 3\n");

    const EnemyConfig config = EnemyConfigLoader::Parse(enemyRoot, "guard");

    ExpectEqual(3, config.guardSegmentCount, "legacy breakCountMax segment count");
    ExpectNear(25.0f, config.guardValuePerSegment, 0.0001f, "common guard value per segment");
}

void WeakBossCombatConfigDoesNotStartBossEncounterPresentation()
{
    const YAML::Node enemyRoot = YAML::Load("enemies:\n"
                                            "  - type: boss_weak\n"
                                            "    isBoss: true\n"
                                            "    isBossEncounter: false\n");

    const EnemyConfig config = EnemyConfigLoader::Parse(enemyRoot, "boss_weak");

    ExpectTrue(config.isBoss, "weak boss combat behavior");
    ExpectFalse(config.isBossEncounter, "weak boss encounter presentation");
}

void BossConfigStartsBossEncounterPresentationByDefault()
{
    const YAML::Node enemyRoot = YAML::Load("enemies:\n"
                                            "  - type: boss_custom\n"
                                            "    isBoss: true\n");

    const EnemyConfig config = EnemyConfigLoader::Parse(enemyRoot, "boss_custom");

    ExpectTrue(config.isBossEncounter, "boss encounter presentation");
}

} // namespace

void RegisterEnemyConfigLoaderTests(TestRegistry& tests)
{
    tests.emplace_back("EnemyConfigLoader.ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig",
                       ParsedEnemyYamlCombinesCommonAndSelectedTypeConfig);
    tests.emplace_back("EnemyConfigLoader.LegacyBreakCountMaxConfiguresGuardSegments",
                       LegacyBreakCountMaxConfiguresGuardSegments);
    tests.emplace_back("EnemyConfigLoader.WeakBossCombatConfigDoesNotStartBossEncounterPresentation",
                       WeakBossCombatConfigDoesNotStartBossEncounterPresentation);
    tests.emplace_back("EnemyConfigLoader.BossConfigStartsBossEncounterPresentationByDefault",
                       BossConfigStartsBossEncounterPresentationByDefault);
}
