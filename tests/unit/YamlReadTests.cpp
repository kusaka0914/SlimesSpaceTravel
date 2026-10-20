#include "support/TestSupport.h"

#include "actor/enemy/EnemyConfigLoader.h"
#include "actor/player/PlayerConfigLoader.h"
#include "utils/YamlRead.h"

#include <yaml-cpp/yaml.h>

namespace {

void ExpectBadConversion(const std::function<void()>& action, const std::string& description)
{
    bool rejected = false;
    try {
        action();
    } catch (const YAML::BadConversion&) {
        rejected = true;
    }
    ExpectTrue(rejected, description);
}

void MissingScalarsKeepConfigurationDefaults()
{
    const auto player = PlayerConfigLoader::Parse(YAML::Load("players: [{}]"));
    ExpectNear(PlayerConfig{}.hp, player.hp, 0.0f, "missing float keeps player default");
    ExpectEqual(PlayerConfig{}.modelPath, player.modelPath, "missing string keeps player default");
    const auto enemy = EnemyConfigLoader::Parse(YAML::Load("enemies: [{type: target}]"), "target");
    ExpectEqual(EnemyConfig{}.guardSegmentCount, enemy.guardSegmentCount, "missing int keeps enemy default");
    ExpectEqual(EnemyConfig{}.isBoss, enemy.isBoss, "missing bool keeps enemy default");
}

void NullAndMalformedNumericScalarsRemainErrors()
{
    for (const auto* value : {"null", "wrong", "[1, 2]", "{value: 1}"}) {
        const auto root = YAML::Load(std::string("players: [{hp: ") + value + "}]");
        ExpectBadConversion([&]() { PlayerConfigLoader::Parse(root); }, "invalid player float is not a fallback");
    }
    for (const auto* value : {"null", "wrong", "1.5", "999999999999999999999999999999999"}) {
        const auto root = YAML::Load(std::string("enemies: [{type: target, breakCountMax: ") + value + "}]");
        ExpectBadConversion([&]() { EnemyConfigLoader::Parse(root, "target"); }, "invalid or overflowing int rejected");
    }
    for (const auto* value : {"null", "wrong", "2"}) {
        const auto root = YAML::Load(std::string("enemies: [{type: target, isBoss: ") + value + "}]");
        ExpectBadConversion([&]() { EnemyConfigLoader::Parse(root, "target"); }, "invalid bool rejected");
    }
}

void EmptyAndNullStringsRetainYamlConversionSemantics()
{
    const auto empty = PlayerConfigLoader::Parse(YAML::Load("players: [{modelPath: ''}]"));
    ExpectTrue(empty.modelPath.empty(), "explicit empty string does not use default");
    const auto null = PlayerConfigLoader::Parse(YAML::Load("players: [{modelPath: null}]"));
    ExpectEqual(std::string("null"), null.modelPath, "yaml-cpp converts explicit null to its string spelling");
    const auto malformed = YAML::Load("enemies: [{type: target, modelPath: [invalid]}]");
    ExpectBadConversion([&]() { EnemyConfigLoader::Parse(malformed, "target"); }, "sequence is not a string");
}

void SharedReaderPreservesFallbackAndConversionContracts()
{
    const auto node = YAML::Load("number: 3.5\nflag: false\nempty: ''\nnullValue: null\ninvalid: [1]\n");
    ExpectNear(3.5f, YamlRead::ReadOr<float>(node, "number", 7), 0.0f, "explicit float type with integer fallback");
    ExpectNear(7.0f, YamlRead::ReadOr<float>(node, "missing", 7), 0.0f, "missing key uses fallback");
    ExpectFalse(YamlRead::ReadOr<bool>(node, "flag", true), "false is a value, not a missing key");
    ExpectEqual(std::string("default"), YamlRead::ReadOr<std::string>(node, "missing", "default"), "string fallback");
    ExpectTrue(YamlRead::ReadOr<std::string>(node, "empty", "default").empty(), "empty string is retained");
    ExpectEqual(std::string("null"), YamlRead::ReadOr<std::string>(node, "nullValue", "default"),
                "null string conversion");
    ExpectBadConversion([&]() { YamlRead::ReadOr<float>(node, "nullValue", 7); },
                        "null numeric value remains an error");
    ExpectBadConversion([&]() { YamlRead::ReadOr<std::string>(node, "invalid", "default"); },
                        "invalid string remains an error");
    ExpectFalse(node["missing"].IsDefined(), "read does not insert a missing key");
}

void SharedReaderDoesNotHideInvalidParentNodes()
{
    bool rejected = false;
    try {
        YamlRead::ReadOr<float>(YAML::Load("scalar"), "key", 7);
    } catch (const YAML::BadSubscript&) {
        rejected = true;
    }
    ExpectTrue(rejected, "invalid parent is not treated as an empty map");
}

} // namespace

void RegisterYamlReadTests(TestRegistry& tests)
{
    tests.emplace_back("YamlRead.SharedReaderContract", SharedReaderPreservesFallbackAndConversionContracts);
    tests.emplace_back("YamlRead.InvalidParentRemainsError", SharedReaderDoesNotHideInvalidParentNodes);
    tests.emplace_back("YamlRead.LoaderMissingScalarsKeepDefaults", MissingScalarsKeepConfigurationDefaults);
    tests.emplace_back("YamlRead.LoaderInvalidScalarsRemainErrors", NullAndMalformedNumericScalarsRemainErrors);
    tests.emplace_back("YamlRead.LoaderStringConversionSemantics", EmptyAndNullStringsRetainYamlConversionSemantics);
}
