#include "TestSupport.h"

#include "system/tutorial/TutorialLibrary.h"

#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
void DeviceSpecificObjectiveTextTakesPriority()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";
    page.keyboardText = "キーボード会話";
    page.objectiveText = "共通目標";
    page.controllerObjectiveText = "ゲームパッド目標";
    page.keyboardObjectiveText = "キーボード目標";

    ExpectEqual(
        std::string("ゲームパッド目標"),
        page.ResolveObjectiveText(true),
        "controller objective text");
    ExpectEqual(
        std::string("キーボード目標"),
        page.ResolveObjectiveText(false),
        "keyboard objective text");
}

void CommonObjectiveTextIsUsedWithoutDeviceVariant()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";
    page.objectiveText = "共通目標";

    ExpectEqual(
        std::string("共通目標"),
        page.ResolveObjectiveText(true),
        "common controller objective text");
    ExpectEqual(
        std::string("共通目標"),
        page.ResolveObjectiveText(false),
        "common keyboard objective text");
}

void ConversationTextIsFallbackForEmptyObjectiveText()
{
    TutorialPage page;
    page.text = "共通会話";
    page.controllerText = "ゲームパッド会話";

    ExpectEqual(
        std::string("ゲームパッド会話"),
        page.ResolveObjectiveText(true),
        "controller conversation fallback");
    ExpectEqual(
        std::string("共通会話"),
        page.ResolveObjectiveText(false),
        "keyboard conversation fallback");
}

void ActionOnlyPageHasNoConversationText()
{
    TutorialPage page;
    page.objectiveText = "スイッチを押して道を開こう";
    page.advanceCondition =
        TutorialAdvanceCondition::PressPressureSwitch;

    ExpectTrue(
        !page.HasConversationText(false),
        "action-only page has no conversation text");
    ExpectEqual(
        std::string("スイッチを押して道を開こう"),
        page.ResolveObjectiveText(false),
        "action-only page keeps its objective text");
}

void LibraryLoadsObjectiveTextAndActionConditions()
{
    const std::filesystem::path fixturePath =
        std::filesystem::temp_directory_path() /
        "space_tutorial_objective_test.yaml";
    {
        std::ofstream fixture(fixturePath);
        fixture
            << "tutorials:\n"
            << "  - id: objective_test\n"
            << "    pages:\n"
            << "      - id: approach\n"
            << "        text: Find switch\n"
            << "        controllerObjectiveText: Controller goal\n"
            << "        keyboardObjectiveText: Keyboard goal\n"
            << "        objectivePlatformId: platform_2\n"
            << "        advance: approachPressureSwitch\n"
            << "      - id: press\n"
            << "        text: Press switch\n"
            << "        objectiveText: Common goal\n"
            << "        advance: pressPressureSwitch\n";
    }

    TutorialLibrary library(fixturePath.string());
    std::filesystem::remove(fixturePath);

    const TutorialDefinition* definition =
        library.Find("objective_test");
    ExpectTrue(definition != nullptr, "loaded tutorial definition");
    ExpectEqual(
        static_cast<std::size_t>(2),
        definition->pages.size(),
        "loaded objective page count");
    ExpectEqual(
        static_cast<int>(TutorialAdvanceCondition::ApproachPressureSwitch),
        static_cast<int>(definition->pages[0].advanceCondition),
        "approach pressure switch condition");
    ExpectEqual(
        std::string("Controller goal"),
        definition->pages[0].ResolveObjectiveText(true),
        "loaded controller objective");
    ExpectEqual(
        std::string("platform_2"),
        definition->pages[0].objectivePlatformId,
        "loaded objective platform id");
    ExpectEqual(
        static_cast<int>(TutorialAdvanceCondition::PressPressureSwitch),
        static_cast<int>(definition->pages[1].advanceCondition),
        "press pressure switch condition");
    ExpectEqual(
        std::string("Common goal"),
        definition->pages[1].ResolveObjectiveText(false),
        "loaded common objective");

    ExpectEqual(
        static_cast<int>(TutorialAdvanceCondition::PlayerSplit),
        static_cast<int>(ParseTutorialAdvanceConditionId("playerSplit")),
        "player split condition");
    ExpectEqual(
        static_cast<int>(TutorialAdvanceCondition::PlayerMerge),
        static_cast<int>(ParseTutorialAdvanceConditionId("playerMerge")),
        "player merge condition");
}
}

void RegisterTutorialObjectiveTextTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "TutorialObjectiveText.DeviceSpecificTextTakesPriority",
        DeviceSpecificObjectiveTextTakesPriority);
    tests.emplace_back(
        "TutorialObjectiveText.CommonTextIsUsedWithoutDeviceVariant",
        CommonObjectiveTextIsUsedWithoutDeviceVariant);
    tests.emplace_back(
        "TutorialObjectiveText.ConversationTextIsFallback",
        ConversationTextIsFallbackForEmptyObjectiveText);
    tests.emplace_back(
        "TutorialObjectiveText.ActionOnlyPageHasNoConversation",
        ActionOnlyPageHasNoConversationText);
    tests.emplace_back(
        "TutorialObjectiveText.LibraryLoadsObjectiveAndConditions",
        LibraryLoadsObjectiveTextAndActionConditions);
}
