#include "TestSupport.h"

#include "gfx/debug/stage/StageYamlRepository.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

class TemporaryYamlFile {
public:
    TemporaryYamlFile()
    {
        const auto uniqueSuffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        mPath = std::filesystem::temp_directory_path() /
            ("space_stage_yaml_test_" +
             std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryYamlFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mPath, removeError);
    }

    const std::filesystem::path& GetPath() const { return mPath; }

private:
    std::filesystem::path mPath;
};

void SaveYamlFilePreservesNestedValues()
{
    TemporaryYamlFile temporaryFile;
    YAML::Node stageConfig;
    stageConfig["enemy"][0]["type"] = "boss";
    stageConfig["enemy"][0]["currentPlanetNum"] = 2;

    const bool wasSaved = StageYamlRepository::SaveYamlFile(
        temporaryFile.GetPath().string(), stageConfig);
    const YAML::Node loadedConfig =
        YAML::LoadFile(temporaryFile.GetPath().string());

    ExpectTrue(wasSaved, "YAML save result");
    ExpectEqual(
        std::string("boss"),
        loadedConfig["enemy"][0]["type"].as<std::string>(),
        "saved enemy type");
    ExpectEqual(
        2,
        loadedConfig["enemy"][0]["currentPlanetNum"].as<int>(),
        "saved planet index");
}

void RemoveSequenceElementRemovesOnlyRequestedIndex()
{
    YAML::Node stageConfig;
    stageConfig["star"].push_back("first");
    stageConfig["star"].push_back("second");
    stageConfig["star"].push_back("third");

    const bool wasRemoved = StageYamlRepository::RemoveSequenceElement(
        stageConfig, "star", 1);

    ExpectTrue(wasRemoved, "remove result");
    ExpectEqual(std::size_t{2}, stageConfig["star"].size(), "sequence size");
    ExpectEqual(
        std::string("first"), stageConfig["star"][0].as<std::string>(),
        "first retained element");
    ExpectEqual(
        std::string("third"), stageConfig["star"][1].as<std::string>(),
        "second retained element");
}

void RemoveSequenceElementRejectsInvalidIndexWithoutMutation()
{
    YAML::Node stageConfig;
    stageConfig["star"].push_back("only");

    const bool wasRemoved = StageYamlRepository::RemoveSequenceElement(
        stageConfig, "star", -1);

    ExpectFalse(wasRemoved, "remove result");
    ExpectEqual(std::size_t{1}, stageConfig["star"].size(), "sequence size");
    ExpectEqual(
        std::string("only"), stageConfig["star"][0].as<std::string>(),
        "retained element");
}

void RemoveSequenceElementRejectsMissingSequence()
{
    YAML::Node stageConfig;

    const bool wasRemoved = StageYamlRepository::RemoveSequenceElement(
        stageConfig, "missing", 0);

    ExpectFalse(wasRemoved, "remove result");
}

void SetSequenceValueUpdatesRequestedElementOnly()
{
    YAML::Node stageConfig;
    stageConfig["enemy"][0]["type"] = "normal";
    stageConfig["enemy"][1]["type"] = "boss";

    const bool wasUpdated = StageYamlRepository::SetSequenceValue(
        stageConfig, "enemy", 1, "type", std::string("normal"));

    ExpectTrue(wasUpdated, "update result");
    ExpectEqual(
        std::string("normal"),
        stageConfig["enemy"][0]["type"].as<std::string>(),
        "unchanged first enemy type");
    ExpectEqual(
        std::string("normal"),
        stageConfig["enemy"][1]["type"].as<std::string>(),
        "updated second enemy type");
}

void SetSequenceValueRejectsOutOfRangeIndexWithoutMutation()
{
    YAML::Node stageConfig;
    stageConfig["enemy"][0]["type"] = "normal";

    const bool wasUpdated = StageYamlRepository::SetSequenceValue(
        stageConfig, "enemy", 2, "type", std::string("boss"));

    ExpectFalse(wasUpdated, "update result");
    ExpectEqual(
        std::string("normal"),
        stageConfig["enemy"][0]["type"].as<std::string>(),
        "retained enemy type");
}

}

void RegisterStageYamlRepositoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "StageYamlRepository.SaveYamlFilePreservesNestedValues",
        SaveYamlFilePreservesNestedValues);
    tests.emplace_back(
        "StageYamlRepository.RemoveSequenceElementRemovesOnlyRequestedIndex",
        RemoveSequenceElementRemovesOnlyRequestedIndex);
    tests.emplace_back(
        "StageYamlRepository.RemoveSequenceElementRejectsInvalidIndexWithoutMutation",
        RemoveSequenceElementRejectsInvalidIndexWithoutMutation);
    tests.emplace_back(
        "StageYamlRepository.RemoveSequenceElementRejectsMissingSequence",
        RemoveSequenceElementRejectsMissingSequence);
    tests.emplace_back(
        "StageYamlRepository.SetSequenceValueUpdatesRequestedElementOnly",
        SetSequenceValueUpdatesRequestedElementOnly);
    tests.emplace_back(
        "StageYamlRepository.SetSequenceValueRejectsOutOfRangeIndexWithoutMutation",
        SetSequenceValueRejectsOutOfRangeIndexWithoutMutation);
}
