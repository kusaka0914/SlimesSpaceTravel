#include "support/TestFileSupport.h"
#include "support/TestSupport.h"

#include "system/tutorial/TutorialLibrary.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <yaml-cpp/yaml.h>

namespace {

// 準備や検証が失敗した場合も、テスト用YAMLを終了時に削除する。
class TemporaryTutorialFile {
public:
    TemporaryTutorialFile()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
        mPath = std::filesystem::temp_directory_path() /
                ("space_tutorial_objective_test_" + std::to_string(uniqueSuffix) + ".yaml");
    }

    ~TemporaryTutorialFile()
    {
        std::error_code removeError;
        std::filesystem::remove(mPath, removeError);
    }

    const std::filesystem::path& GetPath() const { return mPath; }

private:
    std::filesystem::path mPath;
};

void DeviceSpecificObjectiveTextTakesPriority()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";
    page.keyboardText = "キーボード会話";
    page.objectiveText = "共通目標";
    page.controllerObjectiveText = "ゲームパッド目標";
    page.keyboardObjectiveText = "キーボード目標";

    ExpectEqual(std::string("ゲームパッド目標"), page.ResolveObjectiveText(true), "controller objective text");
    ExpectEqual(std::string("キーボード目標"), page.ResolveObjectiveText(false), "keyboard objective text");
}

void CommonObjectiveTextIsUsedWithoutDeviceVariant()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";
    page.objectiveText = "共通目標";

    ExpectEqual(std::string("共通目標"), page.ResolveObjectiveText(true), "common controller objective text");
    ExpectEqual(std::string("共通目標"), page.ResolveObjectiveText(false), "common keyboard objective text");
}

void ConversationTextIsFallbackForEmptyObjectiveText()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";

    ExpectEqual(std::string("ゲームパッド会話"), page.ResolveObjectiveText(true), "controller conversation fallback");
    ExpectEqual(std::string("共通会話"), page.ResolveObjectiveText(false), "keyboard conversation fallback");
}

void ActionOnlyPageHasNoConversationText()
{
    TutorialPage page;
    page.objectiveText = "スイッチを押して道を開こう";
    page.advanceCondition = TutorialAdvanceCondition::PressPressureSwitch;

    ExpectFalse(page.HasConversationText(false), "action-only page has no conversation text");
    ExpectEqual(std::string("スイッチを押して道を開こう"), page.ResolveObjectiveText(false),
                "action-only page keeps its objective text");
}

void LibraryLoadsObjectiveTextAndActionConditions()
{
    const TemporaryTutorialFile fixture;
    const std::filesystem::path& fixturePath = fixture.GetPath();
    WriteTestFile(fixturePath, "tutorials:\n"
                               "  - id: objective_test\n"
                               "    pages:\n"
                               "      - id: reach_side\n"
                               "        objectiveText: Reach the far side\n"
                               "        objectivePlatformId: platform_2\n"
                               "        advance: reachPressureSwitchSide\n"
                               "      - id: approach\n"
                               "        text: Find switch\n"
                               "        controllerObjectiveText: Controller goal\n"
                               "        keyboardObjectiveText: Keyboard goal\n"
                               "        objectivePlatformId: platform_2\n"
                               "        advance: approachPressureSwitch\n"
                               "      - id: press\n"
                               "        text: Press switch\n"
                               "        objectiveText: Common goal\n"
                               "        advance: pressPressureSwitch\n");

    TutorialLibrary library(fixturePath.string());

    const TutorialDefinition* definition = library.Find("objective_test");
    ExpectTrue(definition != nullptr, "loaded tutorial definition");
    ExpectEqual(static_cast<std::size_t>(3), definition->pages.size(), "loaded objective page count");
    ExpectEqual(static_cast<int>(TutorialAdvanceCondition::ReachPressureSwitchSide),
                static_cast<int>(definition->pages[0].advanceCondition), "reach pressure switch side condition");
    ExpectEqual(static_cast<int>(TutorialAdvanceCondition::ApproachPressureSwitch),
                static_cast<int>(definition->pages[1].advanceCondition), "approach pressure switch condition");
    ExpectEqual(std::string("Controller goal"), definition->pages[1].ResolveObjectiveText(true),
                "loaded controller objective");
    ExpectEqual(std::string("platform_2"), definition->pages[1].objectivePlatformId, "loaded objective platform id");
    ExpectEqual(static_cast<int>(TutorialAdvanceCondition::PressPressureSwitch),
                static_cast<int>(definition->pages[2].advanceCondition), "press pressure switch condition");
    ExpectEqual(std::string("Common goal"), definition->pages[2].ResolveObjectiveText(false),
                "loaded common objective");

    ExpectEqual(static_cast<int>(TutorialAdvanceCondition::PlayerSplit),
                static_cast<int>(ParseTutorialAdvanceConditionId("playerSplit")), "player split condition");
    ExpectEqual(static_cast<int>(TutorialAdvanceCondition::PlayerMerge),
                static_cast<int>(ParseTutorialAdvanceConditionId("playerMerge")), "player merge condition");
}

void StandardAndAssistPagesPreserveAllSettingsWhenSaved()
{
    const TemporaryTutorialFile fixture;
    WriteTestFile(fixture.GetPath(), "tutorials: []\n");
    TutorialLibrary library(fixture.GetPath().string());
    TutorialDefinition* definition = library.Add("save_test");
    definition->usesAssistPages = true;

    TutorialPage page;
    page.text = "共通";
    page.controllerText = "操作";
    page.keyboardText = "入力";
    page.objectiveText = "共通目標";
    page.controllerObjectiveText = "パッド目標";
    page.keyboardObjectiveText = "キー目標";
    page.objectivePlatformId = "switch_2";
    page.advanceCondition = TutorialAdvanceCondition::PressPressureSwitch;
    page.focusTarget = {"pressureSwitchPlatforms", 7};
    page.video.assetPath = "videos/custom.mp4";
    page.video.xRatio = 0.1f;
    page.video.yRatio = 0.2f;
    page.video.widthRatio = 0.3f;
    page.video.heightRatio = 0.4f;
    page.video.rotationDegrees = 45.0f;
    page.video.shouldLoop = false;
    page.video.shouldPreserveAspectRatio = false;
    page.video.shouldFlipVertical = false;
    page.rubySegments = {{page.text, "きょうつう", true}};
    page.controllerRubySegments = {{page.controllerText, "そうさ", true}};
    page.keyboardRubySegments = {{page.keyboardText, "にゅうりょく", true}};
    definition->pages = {page};
    definition->pages[0].id = "standard_page";
    definition->assistPages = {page};
    definition->assistPages[0].id = "assist_page";

    ExpectTrue(library.Save(), "save standard and assist pages");
    ExpectTrue(library.Load(), "reload saved standard and assist pages");
    definition = library.Find("save_test");
    ExpectTrue(definition != nullptr, "saved definition exists");
    ExpectTrue(definition->usesAssistPages, "assist pages remain enabled");
    ExpectEqual(std::size_t(1), definition->pages.size(), "standard page count");
    ExpectEqual(std::size_t(1), definition->assistPages.size(), "assist page count");
    ExpectEqual(std::string("standard_page"), definition->pages[0].id, "standard page identity");
    ExpectEqual(std::string("assist_page"), definition->assistPages[0].id, "assist page identity");
    for (const TutorialPage* saved : {&definition->pages[0], &definition->assistPages[0]}) {
        ExpectEqual(page.text, saved->text, "common text");
        ExpectEqual(page.controllerText, saved->controllerText, "controller text");
        ExpectEqual(page.keyboardText, saved->keyboardText, "keyboard text");
        ExpectEqual(page.objectiveText, saved->objectiveText, "common objective");
        ExpectEqual(page.controllerObjectiveText, saved->controllerObjectiveText, "controller objective");
        ExpectEqual(page.keyboardObjectiveText, saved->keyboardObjectiveText, "keyboard objective");
        ExpectEqual(page.objectivePlatformId, saved->objectivePlatformId, "objective platform");
        ExpectEqual(static_cast<int>(page.advanceCondition), static_cast<int>(saved->advanceCondition), "advance");
        ExpectEqual(page.focusTarget.sequenceName, saved->focusTarget.sequenceName, "focus sequence");
        ExpectEqual(page.focusTarget.yamlIndex, saved->focusTarget.yamlIndex, "focus index");
        ExpectEqual(page.video.assetPath, saved->video.assetPath, "video path");
        ExpectNear(page.video.xRatio, saved->video.xRatio, 0.0001f, "video x");
        ExpectNear(page.video.yRatio, saved->video.yRatio, 0.0001f, "video y");
        ExpectNear(page.video.widthRatio, saved->video.widthRatio, 0.0001f, "video width");
        ExpectNear(page.video.heightRatio, saved->video.heightRatio, 0.0001f, "video height");
        ExpectNear(page.video.rotationDegrees, saved->video.rotationDegrees, 0.0001f, "video rotation");
        ExpectFalse(saved->video.shouldLoop, "video loop");
        ExpectFalse(saved->video.shouldPreserveAspectRatio, "video aspect ratio");
        ExpectFalse(saved->video.shouldFlipVertical, "video flip");
        for (const auto& segments : {saved->rubySegments, saved->controllerRubySegments, saved->keyboardRubySegments}) {
            ExpectEqual(std::size_t(1), segments.size(), "custom ruby segment count");
            ExpectTrue(segments[0].showsRuby, "custom ruby is shown");
        }
        ExpectEqual(std::string("きょうつう"), saved->rubySegments[0].reading, "common ruby");
        ExpectEqual(std::string("そうさ"), saved->controllerRubySegments[0].reading, "controller ruby");
        ExpectEqual(std::string("にゅうりょく"), saved->keyboardRubySegments[0].reading, "keyboard ruby");
    }
}

void SaveOmitsUnusedPageSettingsAndDisabledAssistPages()
{
    const TemporaryTutorialFile fixture;
    WriteTestFile(fixture.GetPath(), "tutorials: []\n");
    TutorialLibrary library(fixture.GetPath().string());
    TutorialDefinition* definition = library.Add("optional_settings");
    definition->usesAssistPages = true;
    TutorialPage page;
    page.id = "minimal";
    page.text = "updated text";
    page.rubySegments = {{"old text", "reading", true}};
    definition->pages = {page};
    definition->assistPages = {page};

    ExpectTrue(library.Save(), "save minimal pages");
    const YAML::Node root = YAML::LoadFile(fixture.GetPath().string());
    for (const char* pagesKey : {"pages", "assistPages"}) {
        const YAML::Node saved = root["tutorials"][0][pagesKey][0];
        ExpectEqual(page.text, saved["text"].as<std::string>(), "updated text is saved");
        for (const char* key : {"controllerText", "keyboardText", "objectiveText", "controllerObjectiveText",
                                "keyboardObjectiveText", "objectivePlatformId", "ruby", "focus", "video"}) {
            ExpectFalse(saved[key].IsDefined(), std::string("omit unused setting: ") + key);
        }
    }

    definition->usesAssistPages = false;
    ExpectTrue(library.Save(), "save with assist pages disabled");
    const YAML::Node disabledRoot = YAML::LoadFile(fixture.GetPath().string());
    ExpectFalse(disabledRoot["tutorials"][0]["usesAssistPages"].IsDefined(), "omit disabled assist flag");
    ExpectFalse(disabledRoot["tutorials"][0]["assistPages"].IsDefined(), "omit disabled assist pages");
}
} // namespace

void RegisterTutorialObjectiveTextTests(TestRegistry& tests)
{
    tests.emplace_back("TutorialObjectiveText.DeviceSpecificTextTakesPriority",
                       DeviceSpecificObjectiveTextTakesPriority);
    tests.emplace_back("TutorialObjectiveText.CommonTextIsUsedWithoutDeviceVariant",
                       CommonObjectiveTextIsUsedWithoutDeviceVariant);
    tests.emplace_back("TutorialObjectiveText.ConversationTextIsFallback",
                       ConversationTextIsFallbackForEmptyObjectiveText);
    tests.emplace_back("TutorialObjectiveText.ActionOnlyPageHasNoConversation", ActionOnlyPageHasNoConversationText);
    tests.emplace_back("TutorialObjectiveText.LibraryLoadsObjectiveAndConditions",
                       LibraryLoadsObjectiveTextAndActionConditions);
    tests.emplace_back("TutorialObjectiveText.StandardAndAssistPagesPreserveAllSettingsWhenSaved",
                       StandardAndAssistPagesPreserveAllSettingsWhenSaved);
    tests.emplace_back("TutorialObjectiveText.SaveOmitsUnusedPageSettingsAndDisabledAssistPages",
                       SaveOmitsUnusedPageSettingsAndDisabledAssistPages);
}
