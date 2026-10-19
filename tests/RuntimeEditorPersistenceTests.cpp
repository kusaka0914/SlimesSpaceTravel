#include "RuntimeEditorPersistenceTests.h"

#include "RuntimeStageSupport.h"
#include "TestSupport.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "app/Game.h"
#include "component/PlatformMovementComponent.h"
#include "editor/stage/StageActorYamlWriter.h"

#include <iostream>
#include <yaml-cpp/yaml.h>

namespace {

void ExpectYamlVector(const glm::vec3& expected, const YAML::Node& actual, const std::string& description)
{
    ExpectTrue(actual.IsSequence() && actual.size() == 3, description + " has three coordinates");
    for (int axis = 0; axis < 3; ++axis) {
        ExpectNear(expected[axis], actual[axis].as<float>(), 0.0001f, description);
    }
}

void MovingPlatformSavePreservesAuthoredStartAndSchema(Game& game)
{
    Planet planet(nullptr);
    planet.SetPos({10, 20, 30});
    Platform platform(&game);
    platform.SetCurrentPlanet(&planet);
    platform.SetStageYamlIndex(0);
    planet.AddPlatform(&platform);
    ScopedTestStagePlanets planets(*game.GetCurrentStage(), {&planet});
    DebugEditorContext context{&game};
    StageActorYamlWriter writer(context);
    auto* movement = platform.AddMovementComponent();
    movement->SetBaseLocalPos({1, 2, 3});
    movement->SetDestinationLocalPos({4, 6, 8});
    movement->SetMoveDuration(2.75f);
    movement->SetMoveOnPlayer(true);
    movement->SetReturnDelay(1.25f);
    movement->SetEndpointWaitDurationSeconds(0.5f);

    for (const std::string sequence : {"platforms", "movingPlatforms"}) {
        platform.SetStageSequenceName(sequence);
        platform.ClearEditorAuthoredTransform();
        YAML::Node config;
        YAML::Node actor;
        actor["pos"] = YAML::Load("[99, 98, 97]");
        actor["futureField"] = "retain";
        YAML::Node path = sequence == "movingPlatforms" ? actor : actor["components"]["movement"];
        path["destinationWaitSeconds"] = 19;
        path["futurePathField"] = "retain";
        config[sequence].push_back(actor);
        writer.WriteAllActorStates(config);
        ExpectYamlVector({99, 98, 97}, config[sequence][0]["pos"], "unmarked runtime position is not authored");

        // 終点プレビューの表示位置を保存しても、配置位置は経路の始点に戻す契約。
        platform.SetPos(planet.GetPos() + movement->GetDestinationLocalPos());
        platform.CaptureEditorAuthoredPosition();
        YAML::Node transformsOnly = YAML::Clone(config);
        writer.WriteAllActorStates(config);
        ExpectYamlVector({99, 98, 97}, config[sequence][0]["pos"],
                         "runtime serialization leaves authored transforms for their pass");
        ExpectTrue(writer.WriteEditorAuthoredTransforms(config), "combined runtime and authored serialization");
        ExpectTrue(writer.WriteEditorAuthoredTransforms(transformsOnly), "authored-only serialization");
        for (const YAML::Node& saved : {config[sequence][0], transformsOnly[sequence][0]}) {
            const YAML::Node savedPath = sequence == "movingPlatforms" ? saved : saved["components"]["movement"];
            ExpectYamlVector({1, 2, 3}, saved["pos"], "persist path start rather than endpoint preview");
            ExpectYamlVector({1, 2, 3}, savedPath["startLocalPos"], "path start");
            ExpectYamlVector({4, 6, 8}, savedPath["endLocalPos"], "path destination");
            ExpectYamlVector({3, 4, 5}, savedPath["moveOffset"], "path displacement");
            ExpectNear(2.75f, savedPath["moveDuration"].as<float>(), 0, "duration");
            ExpectTrue(savedPath["moveOnPlayer"].as<bool>(), "player-activated path");
            ExpectNear(1.25f, savedPath["returnDelay"].as<float>(), 0, "return delay");
            ExpectNear(0.5f, savedPath["endpointWaitSeconds"].as<float>(), 0, "endpoint wait");
            ExpectFalse(static_cast<bool>(savedPath["destinationWaitSeconds"]), "obsolete wait key removed");
            ExpectEqual(std::string("retain"), saved["futureField"].as<std::string>(), "unrelated actor field");
            ExpectEqual(std::string("retain"), savedPath["futurePathField"].as<std::string>(), "unrelated path field");
        }
        ExpectTrue(platform.FindEditorAuthoredTransform() != nullptr, "serialization alone keeps authoring marks");
        platform.SetStageYamlIndex(7);
        ExpectFalse(writer.WriteEditorAuthoredTransforms(config), "invalid YAML index rejected");
        ExpectTrue(platform.FindEditorAuthoredTransform() != nullptr, "failed serialization retains authoring marks");
        platform.SetStageYamlIndex(0);
    }
    std::cout << "[PASS] Runtime.MovingPlatformSaveSchemaAndAuthoredStart\n";
}

void NPCTalkSerializationKeepsMetadataAndRemovesStaleKeys(Game& game)
{
    Planet planet(nullptr);
    NPC npc(&game);
    npc.SetCurrentPlanet(&planet);
    npc.SetStageSequenceName("NPCs");
    npc.SetStageYamlIndex(0);
    npc.SetName("案内役");
    npc.SetModelPath("motherSlime.obj");
    npc.SetForcesTalkOnArrival(true);
    npc.AddTalkTexts("旅立ち");
    npc.AddTalkTexts("帰還");
    npc.SetTalkStageClearCondition(1, 3);
    npc.SetTalkCameraFocusTarget(1, "planets", 2);
    npc.SetTalkAdvanceCondition(1, TalkPageAdvanceCondition::Jump);
    npc.SetTalkStartsOpeningAfterPage(1, true);
    npc.SetTalkStartsEndingAfterPage(1, true);
    npc.SetTalkRubySegments(1, {{"帰還", "きかん", true}});
    npc.SetProximityMessageMode(NPCProximityMessageMode::AfterTalk);
    npc.SetTalkProximityMessageText(1, "お帰り");
    npc.SetTalkProximityMessageRubySegments(1, {{"お", "", false}, {"帰り", "かえり", true}});
    planet.AddNPC(&npc);
    ScopedTestStagePlanets planets(*game.GetCurrentStage(), {&planet});
    DebugEditorContext context{&game};
    StageActorYamlWriter writer(context);
    YAML::Node config = YAML::Load("NPCs: [{futureField: retain}]");
    writer.WriteAllActorStates(config);
    const YAML::Node saved = config["NPCs"][0];
    ExpectEqual(std::string("案内役"), saved["name"].as<std::string>(), "NPC name");
    ExpectTrue(saved["forceTalkOnArrival"].as<bool>(), "arrival talk");
    ExpectEqual(std::size_t(2), saved["talkTexts"].size(), "page order retained");
    ExpectEqual(std::string("帰還"), saved["talkTexts"][1].as<std::string>(), "second page text");
    ExpectEqual(1, saved["talkStageClearConditions"][0]["talkIndex"].as<int>(), "condition page index");
    ExpectEqual(3, saved["talkStageClearConditions"][0]["stage"].as<int>(), "stage condition");
    ExpectEqual(std::string("planets"), saved["talkCameraFocus"][0]["sequence"].as<std::string>(), "focus sequence");
    ExpectEqual(2, saved["talkCameraFocus"][0]["index"].as<int>(), "focus target index");
    ExpectEqual(std::string("jump"), saved["talkAdvanceConditions"][0]["condition"].as<std::string>(), "advance ID");
    ExpectEqual(1, saved["talkOpeningAfterPages"][0]["talkIndex"].as<int>(), "opening trigger page");
    ExpectEqual(1, saved["talkEndingAfterPages"][0]["talkIndex"].as<int>(), "ending trigger page");
    ExpectEqual(std::string("きかん"), saved["talkRubies"][0]["segments"][0]["reading"].as<std::string>(), "page ruby");
    ExpectEqual(std::string("afterTalk"), saved["proximityMessage"]["mode"].as<std::string>(), "message mode");
    ExpectEqual(std::string("お帰り"), saved["proximityMessage"]["variants"][0]["text"].as<std::string>(),
                "message text");
    ExpectFalse(static_cast<bool>(saved["proximityMessage"]["rubies"][0]["segments"][0]["reading"]),
                "plain segment has no reading");
    ExpectEqual(std::string("かえり"),
                saved["proximityMessage"]["rubies"][0]["segments"][1]["reading"].as<std::string>(), "message ruby");

    npc.SetTalkStageClearCondition(1, -1);
    npc.ClearTalkCameraFocusTarget(1);
    npc.SetTalkAdvanceCondition(1, TalkPageAdvanceCondition::Confirm);
    npc.SetTalkStartsOpeningAfterPage(1, false);
    npc.SetTalkStartsEndingAfterPage(1, false);
    npc.SetTalkText(1, "帰還しました");
    npc.SetProximityMessageMode(NPCProximityMessageMode::Disabled);
    writer.WriteAllActorStates(config);
    for (const char* key : {"talkStageClearConditions", "talkCameraFocus", "talkAdvanceConditions",
                            "talkOpeningAfterPages", "talkEndingAfterPages", "talkRubies", "proximityMessage"}) {
        ExpectFalse(static_cast<bool>(config["NPCs"][0][key]), std::string("stale key removed: ") + key);
    }
    ExpectEqual(std::string("retain"), config["NPCs"][0]["futureField"].as<std::string>(), "unknown field retained");
    std::cout << "[PASS] Runtime.NPCTalkSaveSchemaAndStaleMetadata\n";
}

} // namespace

void RunRuntimeEditorPersistenceTests(Game& game)
{
    MovingPlatformSavePreservesAuthoredStartAndSchema(game);
    NPCTalkSerializationKeepsMetadataAndRemovesStaleKeys(game);
}
