#include "TestFileSupport.h"
#include "TestProcessSupport.h"
#include "TestSupport.h"

#include "actor/enemy/EnemyPresetRepository.h"
#include "editor/session/EditorSessionRepository.h"
#include "editor/tools/sequence/StorybookEditorPersistence.h"
#include "effect/particle/ParticleConfigLoader.h"
#include "system/UILoadSystem.h"
#include "system/camera/PlayerCameraSettings.h"
#include "system/ending/EndingRollConfig.h"
#include "system/story/StorybookConfig.h"

#include <yaml-cpp/yaml.h>

namespace {

constexpr const char* storybookDialogueYaml =
    "opening: [{id: openingText, type: text, text: [original], futureField: retain}]\n";

void StorybookImageSaveFailureKeepsDialogue()
{
    TemporaryTestDirectory fixture;
    const auto uiPath = fixture.Root() / "ui.yaml";
    const auto imagePath = fixture.Root() / "blocked";
    WriteTestFile(uiPath, storybookDialogueYaml);
    std::filesystem::create_directory(imagePath);
    UILoadSystem ui;
    ui.GetEditableTextInfos()["opening.openingText"].texts = {"変更後の会話"};
    StorybookConfig story;
    story.GetEditablePageImages("opening") = {"変更後.png"};
    std::string error;
    ExpectFalse(StorybookEditorPersistence::Save(ui, story, error, uiPath.string(), imagePath.string()),
                "image destination rejects compound save");
    ExpectTrue(!error.empty(), "compound save reports failure");
    ExpectEqual(std::string(storybookDialogueYaml), ReadTestFile(uiPath), "image failure preserves original dialogue");
    ExpectTrue(std::filesystem::is_directory(imagePath), "invalid image destination remains a directory");
}

void StorybookDialogueFailureKeepsImages()
{
    TemporaryTestDirectory fixture;
    const auto uiPath = fixture.Root() / "ui.yaml";
    const auto imagePath = fixture.Root() / "storybook.yaml";
    const std::string originalImages = "tracks: {opening: {pages: [original.png]}}\n";
    WriteTestFile(uiPath, "opening: [");
    WriteTestFile(imagePath, originalImages);
    UILoadSystem ui;
    StorybookConfig story;
    story.GetEditablePageImages("opening") = {"変更後.png"};
    std::string error;
    ExpectFalse(StorybookEditorPersistence::Save(ui, story, error, uiPath.string(), imagePath.string()),
                "invalid dialogue rejects compound save");
    ExpectTrue(!error.empty(), "dialogue failure is reported");
    ExpectEqual(originalImages, ReadTestFile(imagePath), "dialogue failure preserves original images");
    ExpectEqual(std::string("opening: ["), ReadTestFile(uiPath), "invalid source dialogue is not replaced");
}

void StorybookCompoundSavePreservesPairedContentAndUnknownFields()
{
    TemporaryTestDirectory fixture;
    const auto uiPath = fixture.Root() / "ui.yaml";
    const auto imagePath = fixture.Root() / "story" / "storybook.yaml";
    WriteTestFile(uiPath, storybookDialogueYaml);
    UILoadSystem ui;
    ui.GetEditableTextInfos()["opening.openingText"].texts = {"旅立ち", "帰還"};
    StorybookConfig story;
    story.GetEditablePageImages("opening") = {"旅立ち.png", "帰還.png"};
    std::string error = "old error";
    ExpectTrue(StorybookEditorPersistence::Save(ui, story, error, uiPath.string(), imagePath.string()),
               "save paired storybook content");
    ExpectTrue(error.empty(), "success clears previous error");
    const YAML::Node savedUI = YAML::LoadFile(uiPath.string());
    ExpectEqual(std::string("旅立ち"), savedUI["opening"][0]["text"][0].as<std::string>(), "first page dialogue");
    ExpectEqual(std::string("帰還"), savedUI["opening"][0]["text"][1].as<std::string>(), "second page dialogue");
    ExpectEqual(std::string("retain"), savedUI["opening"][0]["futureField"].as<std::string>(), "unknown UI field");
    StorybookConfig loaded;
    ExpectTrue(loaded.Load(imagePath.string()), "reload saved image mapping");
    ExpectEqual(std::string("旅立ち.png"), loaded.GetPageImage("opening", 0), "first page image");
    ExpectEqual(std::string("帰還.png"), loaded.GetPageImage("opening", 1), "second page image");
    ExpectEqual(std::ptrdiff_t(3),
                std::distance(std::filesystem::recursive_directory_iterator(fixture.Root()),
                              std::filesystem::recursive_directory_iterator()),
                "compound save leaves only two files and their parent directory");
}

#ifndef _WIN32
void StorybookCompoundShortWriteKeepsBothFiles()
{
    TemporaryTestDirectory fixture;
    const auto uiPath = fixture.Root() / "ui.yaml";
    const auto imagePath = fixture.Root() / "storybook.yaml";
    const std::string originalImages = "tracks: {opening: {pages: [original.png]}}\n";
    WriteTestFile(uiPath, storybookDialogueYaml);
    WriteTestFile(imagePath, originalImages);
    UILoadSystem ui;
    ui.GetEditableTextInfos()["opening.openingText"].texts = {"変更後の会話"};
    StorybookConfig story;
    story.GetEditablePageImages("opening") = {"変更後.png"};
    ExpectWriteFailureWithByteLimit(
        [&]() {
            std::string error;
            return StorybookEditorPersistence::Save(ui, story, error, uiPath.string(), imagePath.string());
        },
        8);
    ExpectEqual(std::string(storybookDialogueYaml), ReadTestFile(uiPath), "short write preserves dialogue");
    ExpectEqual(originalImages, ReadTestFile(imagePath), "short write preserves image mapping");
    ExpectEqual(
        std::ptrdiff_t(2),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "short write leaves no temporary or backup files");
}
#endif

void StorybookFailedReloadKeepsTracksAndReferences()
{
    TemporaryTestDirectory fixture;
    StorybookConfig config;
    WriteTestFile(fixture.File(), "tracks: {kept: {pages: [original.png]}}\n");
    ExpectTrue(config.Load(fixture.File().string()), "load original storybook");
    const auto* pages = &config.GetEditablePageImages("kept");
    WriteTestFile(fixture.File(), "tracks: {partial: {pages: [partial.png]}, bad: {pages: [[invalid]]}}\n");
    ExpectFalse(config.Load(fixture.File().string()), "malformed storybook rejected");
    ExpectEqual(std::string("original.png"), config.GetPageImage("kept", 0), "original track retained");
    ExpectTrue(config.GetPageImage("partial", 0).empty(), "partial track not committed");
    ExpectTrue(pages == &config.GetEditablePageImages("kept"), "reference survives failed reload");
}

void EndingFailedReloadKeepsAllFields()
{
    TemporaryTestDirectory fixture;
    EndingRollConfig config;
    config.creditsText = "original";
    config.totalDuration = 42.0f;
    config.imageEvents.push_back(EndingRollImageEvent{.imagePath = "original.png"});
    WriteTestFile(fixture.File(),
                  "credits: {text: partial}\nduration: 12\nimageEvents: [{image: partial.png}, {start: invalid}]\n");
    ExpectFalse(EndingRollConfigIO::Load(config, fixture.File().string()), "malformed ending rejected");
    ExpectEqual(std::string("original"), config.creditsText, "credits retained");
    ExpectNear(42.0f, config.totalDuration, 0.0f, "duration retained");
    ExpectEqual(std::size_t(1), config.imageEvents.size(), "event count retained");
    ExpectEqual(std::string("original.png"), config.imageEvents[0].imagePath, "original event retained");
}

void MissingOptionalConfigurationsStillResetToDefaults()
{
    TemporaryTestDirectory fixture;
    StorybookConfig storybook;
    storybook.GetEditablePageImages("kept").push_back("old.png");
    ExpectTrue(storybook.Load(fixture.File().string()), "missing optional storybook accepted");
    ExpectTrue(storybook.GetPageImage("kept", 0).empty(), "missing optional storybook clears previous tracks");
    EndingRollConfig ending;
    ending.creditsText = "old";
    ending.totalDuration = 100.0f;
    ExpectTrue(EndingRollConfigIO::Load(ending, fixture.File().string()), "missing optional ending accepted");
    ExpectTrue(ending.creditsText.empty(), "missing ending uses default credits");
    ExpectNear(EndingRollConfig{}.totalDuration, ending.totalDuration, 0.0f, "missing ending uses default duration");
}

void StorybookAndEndingRoundTripPreservesAuthoredContent()
{
    TemporaryTestDirectory fixture;
    StorybookConfig story;
    story.GetEditablePageImages("intro") = {"ページ 1.png", "page2.png"};
    ExpectTrue(story.Save(fixture.File().string()), "save storybook");
    StorybookConfig reloadedStory;
    ExpectTrue(reloadedStory.Load(fixture.File().string()), "reload storybook");
    ExpectEqual(std::string("ページ 1.png"), reloadedStory.GetPageImage("intro", 0), "UTF-8 image path");
    ExpectEqual(std::string("page2.png"), reloadedStory.GetPageImage("intro", 99), "last page fallback");
    EndingRollConfig ending;
    ending.creditsText = "制作\nスタッフ";
    ending.totalDuration = 55.0f;
    ending.endImagePath = "end.png";
    EndingRollImageEvent event;
    event.imagePath = "event.png";
    event.repeatCount = 3;
    event.repeatInterval = 5.0f;
    ending.imageEvents.push_back(event);
    ExpectTrue(EndingRollConfigIO::Save(ending, fixture.File().string()), "save ending");
    EndingRollConfig reloadedEnding;
    ExpectTrue(EndingRollConfigIO::Load(reloadedEnding, fixture.File().string()), "reload ending");
    ExpectEqual(ending.creditsText, reloadedEnding.creditsText, "multiline credits");
    ExpectNear(55.0f, reloadedEnding.totalDuration, 0.0f, "authored duration");
    ExpectEqual(ending.endImagePath, reloadedEnding.endImagePath, "end image");
    ExpectEqual(std::size_t(1), reloadedEnding.imageEvents.size(), "image events");
    ExpectEqual(3, reloadedEnding.imageEvents[0].repeatCount, "repeat count");
    ExpectNear(5.0f, reloadedEnding.imageEvents[0].repeatInterval, 0.0f, "repeat interval");
}

void EditorSessionRoundTripPreservesPoseAndSelection()
{
    TemporaryTestDirectory fixture;
    EditorSessionState state;
    state.stageNumber = 4;
    state.stageYamlPath = "stages/ステージ 4.yaml";
    state.sceneCameraPose.position = {1, 2, 3};
    state.hasPlayerDebugPose = true;
    state.playerPosition = {4, 5, 6};
    state.playerPlanetIndex = 2;
    state.selectedActorKeys = {"platform:2", "enemy:1"};
    std::string error;
    ExpectTrue(EditorSessionRepository::Save(fixture.File(), state, error), "save session");
    EditorSessionState loaded;
    ExpectTrue(EditorSessionRepository::Load(fixture.File(), loaded, error), "reload session");
    ExpectEqual(state.stageYamlPath, loaded.stageYamlPath, "stage path");
    ExpectEqual(4, loaded.stageNumber, "stage number");
    ExpectTrue(state.sceneCameraPose.position == loaded.sceneCameraPose.position, "camera pose");
    ExpectTrue(state.playerPosition == loaded.playerPosition, "player pose");
    ExpectEqual(2, loaded.playerPlanetIndex, "player planet binding");
    ExpectEqual(std::size_t(2), loaded.selectedActorKeys.size(), "selection count");
    ExpectEqual(std::string("enemy:1"), loaded.selectedActorKeys[0], "selection is sorted on save");
}

void ParticleRoundTripPreservesEmitterParameters()
{
    TemporaryTestDirectory fixture;
    ParticleEmitterDefinition emitter;
    emitter.texturePath = "textures/particle.png";
    emitter.count = 9;
    emitter.speed = {2.0f, 7.0f};
    emitter.positionOffset = {1, 2, 3};
    emitter.blendMode = ParticleBlendMode::Alpha;
    ParticleEffectDefinition definition;
    definition.displayName = "test effect";
    definition.emitters.push_back(emitter);
    ExpectTrue(ParticleConfigLoader::Save(fixture.File().string(), {{"test", definition}}), "save particle");
    std::unordered_map<std::string, ParticleEffectDefinition> loaded;
    ExpectTrue(ParticleConfigLoader::Load(fixture.File().string(), loaded), "reload particle");
    ExpectEqual(std::size_t(1), loaded.size(), "effect count");
    ExpectEqual(std::size_t(1), loaded.at("test").emitters.size(), "emitter count");
    const auto& actual = loaded.at("test").emitters[0];
    ExpectEqual(9, actual.count, "particle count");
    ExpectNear(2.0f, actual.speed.min, 0.0f, "minimum speed");
    ExpectNear(7.0f, actual.speed.max, 0.0f, "maximum speed");
    ExpectTrue(actual.positionOffset == emitter.positionOffset, "position offset");
    ExpectTrue(actual.blendMode == emitter.blendMode, "blend mode");
}

void EnemyPresetSavePreservesUnrelatedDefinitions()
{
    TemporaryTestDirectory fixture;
    WriteTestFile(
        fixture.File(),
        "unrelated: retained\nenemies: [{type: common, detectionRange: 14}, {type: test, custom: retained}]\n");
    EnemyPresetDefinition preset;
    preset.id = "test";
    preset.hp = 123.0f;
    preset.attacks = {EnemyPresetRepository::CreateDefaultAttack("fanAttack")};
    const auto revision = EnemyPresetRepository::GetRevision();
    std::string error;
    ExpectTrue(EnemyPresetRepository::Save(fixture.File().string(), "test", preset, error), "save preset");
    ExpectEqual(revision + 1, EnemyPresetRepository::GetRevision(), "successful save advances revision once");
    const auto root = YAML::LoadFile(fixture.File().string());
    ExpectEqual(std::string("retained"), root["unrelated"].as<std::string>(), "unknown root field");
    ExpectNear(14.0f, root["enemies"][0]["detectionRange"].as<float>(), 0.0f, "common enemy settings");
    ExpectEqual(std::string("retained"), root["enemies"][1]["custom"].as<std::string>(), "unknown preset field");
    std::vector<EnemyPresetDefinition> loaded;
    ExpectTrue(EnemyPresetRepository::Load(fixture.File().string(), loaded, error), "reload preset");
    ExpectEqual(std::size_t(1), loaded.size(), "common entry excluded");
    ExpectNear(123.0f, loaded[0].hp, 0.0f, "authored hp");
    ExpectEqual(std::string("fanAttack"), loaded[0].attacks[0].type, "attack type");
}

using SaveConfiguration = std::function<bool(const std::filesystem::path&)>;

std::vector<std::pair<std::string, SaveConfiguration>> ConfigurationWriters()
{
    return {
        {"Storybook",
         [](const auto& path) {
             StorybookConfig config;
             config.GetEditablePageImages("test").push_back("page.png");
             return config.Save(path.string());
         }},
        {"Ending",
         [](const auto& path) {
             EndingRollConfig config;
             config.creditsText = "test credits";
             return EndingRollConfigIO::Save(config, path.string());
         }},
        {"Camera", [](const auto& path) { return PlayerCameraSettingsRepository(path.string()).Save({}); }},
        {"EditorSession",
         [](const auto& path) {
             std::string error;
             EditorSessionState config;
             config.stageYamlPath = "stage.yaml";
             return EditorSessionRepository::Save(path, config, error);
         }},
        {"Particle",
         [](const auto& path) {
             ParticleEmitterDefinition emitter;
             emitter.texturePath = "textures/test.png";
             ParticleEffectDefinition definition;
             definition.emitters.push_back(emitter);
             return ParticleConfigLoader::Save(path.string(), {{"test", definition}});
         }},
        {"EnemyPreset",
         [](const auto& path) {
             EnemyPresetDefinition preset;
             preset.id = "test";
             preset.attacks.push_back(EnemyPresetRepository::CreateDefaultAttack("meleeAttack"));
             std::string error;
             return EnemyPresetRepository::Save(path.string(), "test", preset, error);
         }},
    };
}

void ConfigurationWritesProduceValidYaml(const SaveConfiguration& save)
{
    TemporaryTestDirectory fixture;
    WriteTestFile(fixture.File(), "enemies: [{type: test}]\n");
    ExpectTrue(save(fixture.File()), "configuration save succeeds");
    ExpectTrue(YAML::LoadFile(fixture.File().string()).IsMap(), "saved definition is a YAML map");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "no temporary files remain");
}

#ifndef _WIN32
void ConfigurationShortWriteKeepsPreviousBytes(const SaveConfiguration& save)
{
    TemporaryTestDirectory fixture;
    const std::string original = "enemies: [{type: test}]\n";
    WriteTestFile(fixture.File(), original);
    ExpectWriteFailureWithByteLimit([&]() { return save(fixture.File()); }, 8);
    ExpectEqual(original, ReadTestFile(fixture.File()), "old configuration survives failed write");
    ExpectEqual(
        std::ptrdiff_t(1),
        std::distance(std::filesystem::directory_iterator(fixture.Root()), std::filesystem::directory_iterator()),
        "failed write leaves no temporary files");
}
#endif

} // namespace

void RegisterConfigurationPersistenceTests(TestRegistry& tests)
{
    tests.emplace_back("Configuration.StorybookImageFailureKeepsDialogue", StorybookImageSaveFailureKeepsDialogue);
    tests.emplace_back("Configuration.StorybookDialogueFailureKeepsImages", StorybookDialogueFailureKeepsImages);
    tests.emplace_back("Configuration.StorybookCompoundRoundTrip",
                       StorybookCompoundSavePreservesPairedContentAndUnknownFields);
#ifndef _WIN32
    tests.emplace_back("Configuration.StorybookCompoundShortWrite", StorybookCompoundShortWriteKeepsBothFiles);
#endif
    tests.emplace_back("Configuration.StorybookAndEndingRoundTrip",
                       StorybookAndEndingRoundTripPreservesAuthoredContent);
    tests.emplace_back("Configuration.EditorSessionRoundTrip", EditorSessionRoundTripPreservesPoseAndSelection);
    tests.emplace_back("Configuration.ParticleRoundTrip", ParticleRoundTripPreservesEmitterParameters);
    tests.emplace_back("Configuration.EnemyPresetKeepsUnrelatedFields", EnemyPresetSavePreservesUnrelatedDefinitions);
    tests.emplace_back("Configuration.StorybookFailedReloadKeepsTracks", StorybookFailedReloadKeepsTracksAndReferences);
    tests.emplace_back("Configuration.EndingFailedReloadKeepsFields", EndingFailedReloadKeepsAllFields);
    tests.emplace_back("Configuration.MissingOptionalFilesUseDefaults",
                       MissingOptionalConfigurationsStillResetToDefaults);
    for (const auto& [name, save] : ConfigurationWriters()) {
        tests.emplace_back("Configuration." + name + "WritesValidYaml",
                           [save]() { ConfigurationWritesProduceValidYaml(save); });
#ifndef _WIN32
        tests.emplace_back("Configuration." + name + "ShortWriteKeepsOldFile",
                           [save]() { ConfigurationShortWriteKeepsPreviousBytes(save); });
#endif
    }
}
