#include "ActorLoadSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/StageObject.h"
#include "actor/TutorialTrigger.h"
#include "component/PlatformBehaviorComponents.h"
#include "component/PlatformMovementComponent.h"
#include "system/MeshLoadSystem.h"
#include "system/StageActorPlanetBindingService.h"
#include "system/text/JapaneseRubyGenerator.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {

void ApplyPlatformMovementConfig(
    Platform* platform,
    const YAML::Node& movementNode)
{
    if (!platform || !movementNode || !movementNode.IsMap()) {
        return;
    }

    PlatformMovementComponent* movement =
        platform->AddMovementComponent();
    if (!movement) {
        return;
    }

    glm::vec3 baseLocalPos(0.0f);
    if (platform->GetCurrentPlanet()) {
        baseLocalPos =
            platform->GetPos() -
            platform->GetCurrentPlanet()->GetPos();
    }

    if (movementNode["startLocalPos"] &&
        movementNode["startLocalPos"].IsSequence() &&
        movementNode["startLocalPos"].size() >= 3) {
        baseLocalPos.x = movementNode["startLocalPos"][0].as<float>();
        baseLocalPos.y = movementNode["startLocalPos"][1].as<float>();
        baseLocalPos.z = movementNode["startLocalPos"][2].as<float>();
    }
    movement->SetBaseLocalPos(baseLocalPos);

    glm::vec3 moveOffset(4.0f, 0.0f, 0.0f);
    if (movementNode["moveOffset"] &&
        movementNode["moveOffset"].IsSequence() &&
        movementNode["moveOffset"].size() >= 3) {
        moveOffset.x = movementNode["moveOffset"][0].as<float>();
        moveOffset.y = movementNode["moveOffset"][1].as<float>();
        moveOffset.z = movementNode["moveOffset"][2].as<float>();
    }
    movement->SetMoveOffset(moveOffset);

    if (movementNode["endLocalPos"] &&
        movementNode["endLocalPos"].IsSequence() &&
        movementNode["endLocalPos"].size() >= 3) {
        movement->SetDestinationLocalPos(
            glm::vec3(
                movementNode["endLocalPos"][0].as<float>(),
                movementNode["endLocalPos"][1].as<float>(),
                movementNode["endLocalPos"][2].as<float>()));
    }

    movement->SetMoveDuration(
        movementNode["moveDuration"]
            ? movementNode["moveDuration"].as<float>()
            : 3.0f);
    movement->SetMoveOnPlayer(
        movementNode["moveOnPlayer"]
            ? movementNode["moveOnPlayer"].as<bool>()
            : false);
    movement->SetReturnDelay(
        movementNode["returnDelay"]
            ? movementNode["returnDelay"].as<float>()
            : 1.0f);
}

YAML::Node GetMovementComponentNode(const YAML::Node& platformNode)
{
    if (platformNode["components"] &&
        platformNode["components"].IsMap() &&
        platformNode["components"]["movement"] &&
        platformNode["components"]["movement"].IsMap()) {
        return platformNode["components"]["movement"];
    }

    return YAML::Node();
}

glm::vec3 ReadComponentVec3(
    const YAML::Node& node,
    const char* key,
    const glm::vec3& fallback)
{
    if (!node || !node[key] || !node[key].IsSequence() ||
        node[key].size() < 3) {
        return fallback;
    }

    return glm::vec3(
        node[key][0].as<float>(),
        node[key][1].as<float>(),
        node[key][2].as<float>());
}

void ApplyPlatformBehaviorConfigs(
    Platform* platform,
    const YAML::Node& platformNode)
{
    if (!platform || !platformNode["components"] ||
        !platformNode["components"].IsMap()) {
        return;
    }

    const YAML::Node components = platformNode["components"];

    if (const YAML::Node node = components["fadeOnStand"];
        node && node.IsMap()) {
        PlatformFadeOnStandComponent* component =
            platform->AddFadeOnStandComponent();
        component->SetFadeOutDuration(
            node["fadeOutDuration"]
                ? node["fadeOutDuration"].as<float>()
                : 1.0f);
        component->SetReappearDelay(
            node["reappearDelay"]
                ? node["reappearDelay"].as<float>()
                : 2.0f);
    }

    if (const YAML::Node node = components["jumpToggle"];
        node && node.IsMap()) {
        platform->AddJumpToggleComponent()->SetInitiallyVisible(
            node["initiallyVisible"]
                ? node["initiallyVisible"].as<bool>()
                : true);
    }

    if (const YAML::Node node = components["intervalToggle"];
        node && node.IsMap()) {
        PlatformIntervalToggleComponent* component =
            platform->AddIntervalToggleComponent();
        component->SetInitiallyVisible(
            node["initiallyVisible"]
                ? node["initiallyVisible"].as<bool>()
                : true);
        component->SetInterval(
            node["interval"] ? node["interval"].as<float>() : 3.0f);
        component->SetWarningDuration(
            node["warningDuration"]
                ? node["warningDuration"].as<float>()
                : 1.0f);
        component->SetBlinkInterval(
            node["blinkInterval"]
                ? node["blinkInterval"].as<float>()
                : 0.15f);
    }

    if (const YAML::Node node = components["directionalMovement"];
        node && node.IsMap()) {
        platform->AddDirectionalMovementComponent()->SetSpeed(
            node["speed"] ? node["speed"].as<float>() : 2.0f);
    }

    if (const YAML::Node node = components["rotation"];
        node && node.IsMap()) {
        PlatformRotationComponent* component =
            platform->AddRotationComponent();
        component->SetLocalAxis(
            ReadComponentVec3(
                node,
                "axis",
                glm::vec3(0.0f, 1.0f, 0.0f)));
        component->SetDegreesPerSecond(
            node["degreesPerSecond"]
                ? node["degreesPerSecond"].as<float>()
                : 45.0f);
    }

    if (const YAML::Node node = components["conveyor"];
        node && node.IsMap()) {
        PlatformConveyorComponent* component =
            platform->AddConveyorComponent();
        component->SetLocalDirection(
            ReadComponentVec3(
                node,
                "direction",
                glm::vec3(0.0f, 0.0f, 1.0f)));
        component->SetSpeed(
            node["speed"] ? node["speed"].as<float>() : 2.0f);
    }

    if (const YAML::Node node = components["pressureSwitch"];
        node && node.IsMap()) {
        std::vector<std::string> targetPlatformIds;
        const YAML::Node targets = node["targets"];
        if (targets && targets.IsSequence()) {
            targetPlatformIds.reserve(targets.size());
            for (const YAML::Node& target : targets) {
                if (target && target.IsScalar()) {
                    targetPlatformIds.emplace_back(
                        target.as<std::string>());
                }
            }
        }
        PlatformPressureSwitchComponent* component =
            platform->AddPressureSwitchComponent();
        component->SetInactiveOpacity(
            node["inactiveOpacity"]
                ? node["inactiveOpacity"].as<float>()
                : 0.2f);
        component->SetTargetPlatformIds(targetPlatformIds);
    }

    if (const YAML::Node node = components["latchedGroupSwitch"];
        node && node.IsMap()) {
        std::vector<PlatformRevealTarget> revealTargets;
        const YAML::Node targets = node["targets"];
        if (targets && targets.IsSequence()) {
            revealTargets.reserve(targets.size());
            for (const YAML::Node& targetNode : targets) {
                if (!targetNode || !targetNode.IsMap() ||
                    !targetNode["sequence"] ||
                    !targetNode["index"]) {
                    continue;
                }

                PlatformRevealTarget target;
                target.sequenceName =
                    targetNode["sequence"].as<std::string>();
                target.yamlIndex = targetNode["index"].as<int>();
                if (target.IsValid()) {
                    revealTargets.emplace_back(std::move(target));
                }
            }
        }

        PlatformLatchedGroupSwitchComponent* component =
            platform->AddLatchedGroupSwitchComponent();
        component->SetGroupId(
            node["groupId"]
                ? node["groupId"].as<std::string>()
                : std::string());
        component->SetRevealTargets(revealTargets);
    }
}

void ApplyTalkPageAdvanceConditions(
    NPC* npc,
    const YAML::Node& actorNode)
{
    if (!npc || !actorNode["talkAdvanceConditions"] ||
        !actorNode["talkAdvanceConditions"].IsSequence()) {
        return;
    }

    for (const YAML::Node& conditionNode :
         actorNode["talkAdvanceConditions"]) {
        if (!conditionNode.IsMap() ||
            !conditionNode["talkIndex"] ||
            !conditionNode["condition"]) {
            continue;
        }

        const int talkIndex =
            conditionNode["talkIndex"].as<int>();
        if (talkIndex < 0) {
            continue;
        }

        npc->SetTalkAdvanceCondition(
            static_cast<std::size_t>(talkIndex),
            ParseTalkPageAdvanceConditionId(
                conditionNode["condition"].as<std::string>()));
    }
}

} // namespace

ActorLoadSystem::ActorLoadSystem(Game* game)
    : mGame(game),
      mActorFactory(game, mPlacementLoader)
{
}

void ActorLoadSystem::LoadData(bool isLoadPlayer)
{
    (void)isLoadPlayer;

    if (!mGame) {
        return;
    }

    const std::string& path = mGame->GetCurrentStageYamlPath();

    LoadPlanets(path.c_str());
    LoadEnemies(path.c_str());
    LoadBoatArrivalPoints(path.c_str());
    LoadBoats(path.c_str());
    LoadBoatParts(path.c_str());
    LoadKeys(path.c_str());
    LoadCrystals(path.c_str());
    LoadStar(path.c_str());
    LoadNPCs(path.c_str());
    LoadTutorialTriggers(path.c_str());
    LoadPlatforms(path.c_str());
    LoadLegacyMovingPlatforms(path.c_str());
    LoadStageObjects(path.c_str());
    LoadFallRespawnPoints(path.c_str());
    LoadPlayers(path.c_str());

    // Older stage YAML can contain a stale or omitted currentPlanetNum. The
    // world-space placement remains authoritative, so reconcile ownership
    // only after every category and planet has finished loading.
    StageActorPlanetBindingService::RefreshNearestPlanetBindings(
        mGame->GetCurrentStage());
}

void ActorLoadSystem::LoadPlayers(const char* path)
{
    YAML::Node root = YAML::LoadFile(path);

    mGame->RemoveAllPlayer();

    YAML::Node playerNodes = root["players"];
    if (!playerNodes || !playerNodes.IsSequence() || playerNodes.size() == 0) {
        playerNodes = YAML::Node(YAML::NodeType::Sequence);
        YAML::Node defaultPlayer;
        defaultPlayer["currentPlanetNum"] = 0;
        playerNodes.push_back(defaultPlayer);
    }

    constexpr int requiredPlayerCount = 2;
    int loadedPlayerCount = 0;
    for (const YAML::Node& node : playerNodes) {
        if (loadedPlayerCount >= requiredPlayerCount) {
            break;
        }

        const int playerNum = loadedPlayerCount + 1;
        if (CreatePlayerFromStageNode(node, playerNum)) {
            loadedPlayerCount++;
        }
    }

    if (loadedPlayerCount >= requiredPlayerCount) {
        return;
    }

    const YAML::Node fallbackNode = playerNodes[0];
    while (loadedPlayerCount < requiredPlayerCount) {
        const int playerNum = loadedPlayerCount + 1;
        Player* duplicatedPlayer =
            CreatePlayerFromStageNode(fallbackNode, playerNum);
        if (!duplicatedPlayer) {
            break;
        }

        const std::vector<Player*>& players = mGame->GetPlayers();
        Player* firstPlayer = players.empty() ? nullptr : players[0];
        if (firstPlayer && duplicatedPlayer != firstPlayer) {
            glm::vec3 separationDirection = firstPlayer->GetLeftVec();
            const float separationLength = glm::length(separationDirection);
            if (separationLength > 0.000001f) {
                separationDirection /= separationLength;
                constexpr float duplicateSpawnSpacing = 1.0f;
                duplicatedPlayer->SetPos(
                    firstPlayer->GetPos() +
                    separationDirection * duplicateSpawnSpacing);
                duplicatedPlayer->Initialize();
                duplicatedPlayer->RefreshFallbackUpVec();
            }
        }

        loadedPlayerCount++;
    }
}

Player* ActorLoadSystem::CreatePlayerFromStageNode(const YAML::Node& node, int playerNum)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];

    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Player> player = std::make_unique<Player>(mGame);

    player->SetPlayerNum(playerNum);
    player->SetCurrentPlanetNum(currentPlanetNum);
    player->SetCurrentPlanet(currentPlanet);

    mPlacementLoader.ApplyPlacementFromStageNode(player.get(), node, currentPlanet, playerNum - 1, 0.0f);
    mPlacementLoader.ApplyRotationFromStageNode(player.get(), node);

    player->ApplyConfig();

    if (node["modelPath"]) {
        player->SetModelPath(node["modelPath"].as<std::string>());
    }

    mPlacementLoader.ApplyScaleFromStageNode(player.get(), node);

    player->Initialize();
    player->SetBaseScale(player->GetScale());

    Player* playerPtr = player.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(playerPtr);
    mGame->AddActor(std::move(player));
    mGame->AddPlayer(playerPtr);

    return playerPtr;
}

bool ActorLoadSystem::CreatePlayerFromCurrentStage(int playerNum)
{
    if (!mGame) {
        return false;
    }

    const std::string& path = mGame->GetCurrentStageYamlPath();

    YAML::Node root = YAML::LoadFile(path);

    if (!root["players"] || !root["players"].IsSequence()) {
        return false;
    }

    const int playerIndex = playerNum - 1;

    if (playerIndex < 0 || playerIndex >= static_cast<int>(root["players"].size())) {
        return false;
    }

    CreatePlayerFromStageNode(root["players"][playerIndex], playerNum);
    return true;
}

void ActorLoadSystem::LoadNPCs(const char* path)
{
    mActorFactory.LoadActorSequence<NPC>(
        path, "NPCs", [](Planet* planet) { planet->RemoveAllNPCs(); },
        [this](const YAML::Node& node, int index) { return CreateNPCFromStageNode(node, index); });
}

NPC* ActorLoadSystem::CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<NPC>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "npc.obj", [](Planet* planet, NPC* npc) { planet->AddNPC(npc); },
        [](NPC* npc, const YAML::Node& node) {
            const float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
            npc->SetFacingYaw(facingYaw);

            const float radius = node["radius"] ? node["radius"].as<float>() : 0.75f;
            npc->SetRadius(radius);

            const std::string name = node["name"] ? node["name"].as<std::string>() : "";
            npc->SetName(name);

            if (node["proximityMessage"] &&
                node["proximityMessage"].IsMap()) {
                const YAML::Node messageNode = node["proximityMessage"];
                const std::string mode =
                    messageNode["mode"]
                        ? messageNode["mode"].as<std::string>()
                        : "disabled";
                if (mode == "afterTalk") {
                    npc->SetProximityMessageMode(
                        NPCProximityMessageMode::AfterTalk);
                } else if (mode == "always") {
                    npc->SetProximityMessageMode(
                        NPCProximityMessageMode::Always);
                } else {
                    npc->SetProximityMessageMode(
                        NPCProximityMessageMode::Disabled);
                }

                npc->SetProximityMessageRange(
                    messageNode["range"]
                        ? messageNode["range"].as<float>()
                        : 3.0f);
                npc->SetProximityMessageHeight(
                    messageNode["height"]
                        ? messageNode["height"].as<float>()
                        : 1.8f);
                npc->SetProximityMessageScale(
                    messageNode["scale"]
                        ? messageNode["scale"].as<float>()
                        : 1.0f);
            }

            if (node["talkTexts"] && node["talkTexts"].IsSequence()) {
                for (const YAML::Node& talkTextNode : node["talkTexts"]) {
                    npc->AddTalkTexts(talkTextNode.as<std::string>());
                }
            }

            if (npc->GetTalkTexts().empty() &&
                npc->GetProximityMessageMode() !=
                    NPCProximityMessageMode::Disabled) {
                npc->AddTalkTexts("");
            }

            ApplyTalkPageAdvanceConditions(npc, node);

            if (node["proximityMessage"] &&
                node["proximityMessage"].IsMap()) {
                const YAML::Node messageNode = node["proximityMessage"];
                bool loadedVariants = false;
                if (messageNode["variants"] &&
                    messageNode["variants"].IsSequence()) {
                    for (const YAML::Node& variantNode :
                         messageNode["variants"]) {
                        if (!variantNode.IsMap() ||
                            !variantNode["talkIndex"] ||
                            !variantNode["text"]) {
                            continue;
                        }

                        const int talkIndex =
                            variantNode["talkIndex"].as<int>();
                        if (talkIndex < 0) {
                            continue;
                        }
                        npc->SetTalkProximityMessageText(
                            static_cast<std::size_t>(talkIndex),
                            variantNode["text"].as<std::string>());
                        loadedVariants = true;
                    }
                }

                // Compatibility with the first implementation, which stored
                // one shared message for every clear-state conversation.
                if (!loadedVariants && messageNode["text"]) {
                    const std::string legacyText =
                        messageNode["text"].as<std::string>();
                    for (std::size_t talkIndex = 0;
                         talkIndex < npc->GetTalkTexts().size();
                         ++talkIndex) {
                        npc->SetTalkProximityMessageText(
                            talkIndex, legacyText);
                    }
                }

                if (messageNode["rubies"] &&
                    messageNode["rubies"].IsSequence()) {
                    for (const YAML::Node& rubyNode :
                         messageNode["rubies"]) {
                        if (!rubyNode.IsMap() ||
                            !rubyNode["talkIndex"] ||
                            !rubyNode["segments"] ||
                            !rubyNode["segments"].IsSequence()) {
                            continue;
                        }

                        const int talkIndex =
                            rubyNode["talkIndex"].as<int>();
                        if (talkIndex < 0) {
                            continue;
                        }

                        std::vector<RubyTextSegment> segments;
                        for (const YAML::Node& segmentNode :
                             rubyNode["segments"]) {
                            if (!segmentNode.IsMap() ||
                                !segmentNode["text"]) {
                                continue;
                            }

                            RubyTextSegment segment;
                            segment.text =
                                segmentNode["text"].as<std::string>();
                            segment.reading =
                                segmentNode["reading"]
                                    ? segmentNode["reading"].as<std::string>()
                                    : std::string();
                            segment.showsRuby =
                                segmentNode["ruby"]
                                    ? segmentNode["ruby"].as<bool>()
                                    : !segment.reading.empty();
                            segments.emplace_back(
                                std::move(segment));
                        }

                        npc->SetTalkProximityMessageRubySegments(
                            static_cast<std::size_t>(talkIndex),
                            std::move(segments));
                    }
                }

                for (std::size_t talkIndex = 0;
                     talkIndex < npc->GetTalkTexts().size();
                     ++talkIndex) {
                    const std::string& proximityText =
                        npc->GetTalkProximityMessageText(talkIndex);
                    if (proximityText.empty() ||
                        npc->HasValidTalkProximityMessageRuby(
                            talkIndex)) {
                        continue;
                    }

                    std::vector<RubyTextSegment> generatedSegments;
                    std::string errorMessage;
                    if (JapaneseRubyGenerator::Generate(
                            proximityText,
                            generatedSegments,
                            errorMessage)) {
                        npc->SetTalkProximityMessageRubySegments(
                            talkIndex,
                            std::move(generatedSegments));
                    }
                }
            }

            if (node["talkStageClearConditions"] &&
                node["talkStageClearConditions"].IsSequence()) {
                for (const YAML::Node& conditionNode :
                     node["talkStageClearConditions"]) {
                    if (!conditionNode.IsMap() ||
                        !conditionNode["talkIndex"] ||
                        !conditionNode["stage"]) {
                        continue;
                    }

                    const int talkIndex =
                        conditionNode["talkIndex"].as<int>();
                    const int stageNum = conditionNode["stage"].as<int>();
                    if (talkIndex < 0 || stageNum < 0) {
                        continue;
                    }
                    npc->SetTalkStageClearCondition(
                        static_cast<std::size_t>(talkIndex), stageNum);
                }
            }

            if (node["talkCameraFocus"] && node["talkCameraFocus"].IsSequence()) {
                for (const YAML::Node& focusNode : node["talkCameraFocus"]) {
                    if (!focusNode.IsMap() || !focusNode["talkIndex"] ||
                        !focusNode["sequence"] || !focusNode["index"]) {
                        continue;
                    }

                    const int talkIndex = focusNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    npc->SetTalkCameraFocusTarget(
                        static_cast<std::size_t>(talkIndex),
                        focusNode["sequence"].as<std::string>(),
                        focusNode["index"].as<int>());
                }
            }

            if (node["talkRubies"] && node["talkRubies"].IsSequence()) {
                for (const YAML::Node& rubyNode : node["talkRubies"]) {
                    if (!rubyNode.IsMap() || !rubyNode["talkIndex"] ||
                        !rubyNode["segments"] || !rubyNode["segments"].IsSequence()) {
                        continue;
                    }

                    const int talkIndex = rubyNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    std::vector<RubyTextSegment> segments;
                    for (const YAML::Node& segmentNode : rubyNode["segments"]) {
                        if (!segmentNode.IsMap() || !segmentNode["text"]) {
                            continue;
                        }

                        RubyTextSegment segment;
                        segment.text = segmentNode["text"].as<std::string>();
                        segment.reading =
                            segmentNode["reading"]
                                ? segmentNode["reading"].as<std::string>()
                                : std::string();
                        segment.showsRuby =
                            segmentNode["ruby"]
                                ? segmentNode["ruby"].as<bool>()
                                : !segment.reading.empty();
                        segments.emplace_back(std::move(segment));
                    }

                    npc->SetTalkRubySegments(
                        static_cast<std::size_t>(talkIndex),
                        std::move(segments));
                }
            }

            const std::vector<std::string>& talkTexts = npc->GetTalkTexts();
            for (std::size_t talkIndex = 0; talkIndex < talkTexts.size(); ++talkIndex) {
                if (npc->HasValidTalkRuby(talkIndex)) {
                    continue;
                }

                std::vector<RubyTextSegment> generatedSegments;
                std::string errorMessage;
                if (JapaneseRubyGenerator::Generate(
                        talkTexts[talkIndex], generatedSegments, errorMessage)) {
                    npc->SetTalkRubySegments(
                        talkIndex, std::move(generatedSegments));
                }
            }

            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            npc->ApplyConfig(type);
        },
        [](NPC* npc, const YAML::Node&) { npc->SetBaseScale(npc->GetScale()); });
}

void ActorLoadSystem::LoadTutorialTriggers(const char* path)
{
    mActorFactory.LoadActorSequence<TutorialTrigger>(
        path,
        "tutorialTriggers",
        [](Planet* planet) { planet->RemoveAllTutorialTriggers(); },
        [this](const YAML::Node& node, int index) {
            return CreateTutorialTriggerFromStageNode(node, index);
        });
}

TutorialTrigger* ActorLoadSystem::CreateTutorialTriggerFromStageNode(
    const YAML::Node& node,
    int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<TutorialTrigger>(
        node,
        stageYamlIndex,
        1.0f,
        glm::vec3(2.0f),
        "selectField.obj",
        [](Planet* planet, TutorialTrigger* trigger) {
            planet->AddTutorialTrigger(trigger);
        },
        [](TutorialTrigger* trigger, const YAML::Node& triggerNode) {
            trigger->SetTutorialId(
                triggerNode["tutorialId"]
                    ? triggerNode["tutorialId"].as<std::string>()
                    : std::string());

            if (triggerNode["talkTexts"] &&
                triggerNode["talkTexts"].IsSequence()) {
                for (const YAML::Node& textNode :
                     triggerNode["talkTexts"]) {
                    trigger->AddTalkTexts(
                        textNode.as<std::string>());
                }
            }
            if (trigger->GetTalkTexts().empty()) {
                trigger->AddTalkTexts("");
            }

            ApplyTalkPageAdvanceConditions(
                trigger,
                triggerNode);

            if (triggerNode["talkStageClearConditions"] &&
                triggerNode["talkStageClearConditions"].IsSequence()) {
                for (const YAML::Node& conditionNode :
                     triggerNode["talkStageClearConditions"]) {
                    if (!conditionNode.IsMap() ||
                        !conditionNode["talkIndex"] ||
                        !conditionNode["stage"]) {
                        continue;
                    }

                    const int talkIndex =
                        conditionNode["talkIndex"].as<int>();
                    const int stageNum =
                        conditionNode["stage"].as<int>();
                    if (talkIndex < 0 || stageNum < 0) {
                        continue;
                    }

                    trigger->SetTalkStageClearCondition(
                        static_cast<std::size_t>(talkIndex),
                        stageNum);
                }
            }

            if (triggerNode["talkCameraFocus"] &&
                triggerNode["talkCameraFocus"].IsSequence()) {
                for (const YAML::Node& focusNode :
                     triggerNode["talkCameraFocus"]) {
                    if (!focusNode.IsMap() ||
                        !focusNode["talkIndex"] ||
                        !focusNode["sequence"] ||
                        !focusNode["index"]) {
                        continue;
                    }

                    const int talkIndex =
                        focusNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    trigger->SetTalkCameraFocusTarget(
                        static_cast<std::size_t>(talkIndex),
                        focusNode["sequence"].as<std::string>(),
                        focusNode["index"].as<int>());
                }
            }

            if (triggerNode["talkRubies"] &&
                triggerNode["talkRubies"].IsSequence()) {
                for (const YAML::Node& rubyNode :
                     triggerNode["talkRubies"]) {
                    if (!rubyNode.IsMap() ||
                        !rubyNode["talkIndex"] ||
                        !rubyNode["segments"] ||
                        !rubyNode["segments"].IsSequence()) {
                        continue;
                    }

                    const int talkIndex =
                        rubyNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    std::vector<RubyTextSegment> segments;
                    for (const YAML::Node& segmentNode :
                         rubyNode["segments"]) {
                        if (!segmentNode.IsMap() ||
                            !segmentNode["text"]) {
                            continue;
                        }

                        RubyTextSegment segment;
                        segment.text =
                            segmentNode["text"].as<std::string>();
                        segment.reading =
                            segmentNode["reading"]
                                ? segmentNode["reading"].as<std::string>()
                                : std::string();
                        segment.showsRuby =
                            segmentNode["ruby"]
                                ? segmentNode["ruby"].as<bool>()
                                : !segment.reading.empty();
                        segments.emplace_back(std::move(segment));
                    }
                    trigger->SetTalkRubySegments(
                        static_cast<std::size_t>(talkIndex),
                        std::move(segments));
                }
            }

            const std::vector<std::string>& talkTexts =
                trigger->GetTalkTexts();
            for (std::size_t talkIndex = 0;
                 talkIndex < talkTexts.size();
                 ++talkIndex) {
                if (trigger->HasValidTalkRuby(talkIndex)) {
                    continue;
                }

                std::vector<RubyTextSegment> generatedSegments;
                std::string errorMessage;
                if (JapaneseRubyGenerator::Generate(
                        talkTexts[talkIndex],
                        generatedSegments,
                        errorMessage)) {
                    trigger->SetTalkRubySegments(
                        talkIndex,
                        std::move(generatedSegments));
                }
            }
        },
        [](TutorialTrigger* trigger, const YAML::Node& triggerNode) {
            if ((!triggerNode["scale"] ||
                 !triggerNode["scale"].IsSequence()) &&
                triggerNode["radius"]) {
                const float legacyRadius =
                    std::max(
                        0.01f,
                        triggerNode["radius"].as<float>());
                trigger->SetScale(
                    glm::vec3(legacyRadius));
            }
        });
}

Actor* ActorLoadSystem::FindPlacedActor(const std::string& sequenceName, int stageYamlIndex) const
{
    if (!mGame || stageYamlIndex < 0) {
        return nullptr;
    }

    Stage* stage = mGame->GetCurrentStage();
    if (!stage) {
        return nullptr;
    }

    const auto findByIndex = [stageYamlIndex](const auto& actors) -> Actor* {
        for (auto* actor : actors) {
            if (actor && actor->GetStageYamlIndex() == stageYamlIndex) {
                return actor;
            }
        }
        return nullptr;
    };

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (platform &&
                platform->GetStageSequenceName() == sequenceName &&
                platform->GetStageYamlIndex() == stageYamlIndex) {
                return platform;
            }
        }

        if (sequenceName == "enemies") {
            if (Actor* actor = findByIndex(planet->GetEnemies())) return actor;
        } else if (sequenceName == "boats") {
            if (Actor* actor = findByIndex(planet->GetBoats())) return actor;
        } else if (sequenceName == "boatParts") {
            if (Actor* actor = findByIndex(planet->GetBoatParts())) return actor;
        } else if (sequenceName == "crystals") {
            if (Actor* actor = findByIndex(planet->GetCrystals())) return actor;
        } else if (sequenceName == "NPCs") {
            if (Actor* actor = findByIndex(planet->GetNPCs())) return actor;
        } else if (sequenceName == "tutorialTriggers") {
            if (Actor* actor = findByIndex(planet->GetTutorialTriggers())) return actor;
        } else if (sequenceName == "boatArrivalPoints") {
            if (Actor* actor = findByIndex(planet->GetBoatArrivalPoints())) return actor;
        } else if (sequenceName == "fallRespawnPoints") {
            if (Actor* actor = findByIndex(planet->GetFallRespawnPoints())) return actor;
        } else if (sequenceName == "stageObjects") {
            if (Actor* actor = findByIndex(planet->GetStageObjects())) return actor;
        } else if (sequenceName == "keys") {
            Key* key = planet->GetKey();
            if (key && key->GetStageYamlIndex() == stageYamlIndex) return key;
        } else if (sequenceName == "star") {
            Star* star = planet->GetStar();
            if (star && star->GetStageYamlIndex() == stageYamlIndex) return star;
        }
    }

    return nullptr;
}

void ActorLoadSystem::LoadEnemies(const char* path)
{
    mActorFactory.LoadActorSequence<Enemy>(
        path, "enemies", [](Planet* planet) { planet->RemoveAllEnemy(); },
        [this](const YAML::Node& node, int index) { return CreateEnemyFromStageNode(node, index); });
}

Enemy* ActorLoadSystem::CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Enemy>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "enemy.obj",
        [](Planet* planet, Enemy* enemy) { planet->AddEnemy(enemy); },
        [](Enemy* enemy, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "normal";
            enemy->ApplyConfig(type);
        },
        [](Enemy* enemy, const YAML::Node&) { enemy->SetBaseScale(enemy->GetScale()); });
}

void ActorLoadSystem::LoadPlanets(const char* path)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    mGame->GetCurrentStage()->RemoveAllPlanet();

    YAML::Node root = YAML::LoadFile(path);

    if (!root["planets"] || !root["planets"].IsSequence()) {
        return;
    }

    for (const YAML::Node& node : root["planets"]) {
        CreatePlanetFromStageNode(node);
    }
}

Planet* ActorLoadSystem::CreatePlanetFromStageNode(const YAML::Node& node)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    std::unique_ptr<Planet> planet = std::make_unique<Planet>(mGame);

    Stage* currentStage = mGame->GetCurrentStage();

    planet->SetCurrentStage(currentStage);
    planet->ApplyConfig(node);

    planet->Initialize();

    Planet* planetPtr = planet.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(planetPtr);
    mGame->AddActor(std::move(planet));
    currentStage->AddPlanet(planetPtr);

    return planetPtr;
}

void ActorLoadSystem::LoadBoats(const char* path)
{
    mActorFactory.LoadActorSequence<Boat>(
        path, "boats", [](Planet* planet) { planet->RemoveAllBoat(); },
        [this](const YAML::Node& node, int index) { return CreateBoatFromStageNode(node, index); });
}

Boat* ActorLoadSystem::CreateBoatFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int startPlanetNum = node["startPlanet"] ? node["startPlanet"].as<int>() : 0;
    const int destPlanetNum = node["destPlanet"] ? node["destPlanet"].as<int>() : 0;

    if (startPlanetNum < 0 || startPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    if (destPlanetNum < 0 || destPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[startPlanetNum];
    Planet* destPlanet = planets[destPlanetNum];

    if (!currentPlanet || !destPlanet) {
        return nullptr;
    }

    std::unique_ptr<Boat> boat = std::make_unique<Boat>(mGame);

    boat->SetCurrentPlanet(currentPlanet);
    boat->SetDestPlanet(destPlanet);

    const int arrivalPointIndex = node["arrivalPointIndex"] ? node["arrivalPointIndex"].as<int>() : -1;

    if (arrivalPointIndex >= 0) {
        for (BoatArrivalPoint* point : destPlanet->GetBoatArrivalPoints()) {
            if (point && point->GetStageYamlIndex() == arrivalPointIndex) {
                boat->SetArrivalPoint(point);
                break;
            }
        }
    }

    const int destStage = node["destStage"] ? node["destStage"].as<int>() : 0;
    boat->SetDestStage(destStage);

    const bool hasTravelSpeed = static_cast<bool>(node["travelSpeed"]);
    const float travelSpeed =
        hasTravelSpeed ? node["travelSpeed"].as<float>() : 10.0f;
    const bool hasLegacyTravelDuration =
        static_cast<bool>(node["travelDuration"]);
    const float legacyTravelDuration =
        hasLegacyTravelDuration
            ? node["travelDuration"].as<float>()
            : 3.0f;

    const float destMargin =
        node["destMargin"] ? node["destMargin"].as<float>() : 4.0f;
    boat->SetDestMargin(destMargin);

    const std::string launchSequenceId =
        node["launchSequenceId"]
            ? node["launchSequenceId"].as<std::string>()
            : std::string("launch_rocket_from_base");
    boat->SetLaunchSequenceId(launchSequenceId);

    const float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
    boat->SetFacingYaw(facingYaw);

    mPlacementLoader.ApplyPlacementFromStageNode(boat.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    mPlacementLoader.ApplyRotationFromStageNode(boat.get(), node);

    YAML::Node boatRoot = YAML::LoadFile("../assets/data/actor/boats.yaml");
    for (const YAML::Node& boatNode : boatRoot["boats"]) {
        const std::string modelPath = boatNode["modelPath"] ? boatNode["modelPath"].as<std::string>() : "";
        boat->SetModelPath(modelPath);

        const float scale = boatNode["scale"] ? boatNode["scale"].as<float>() : 0.25f;
        boat->SetScale(glm::vec3(scale));
    }

    if (node["modelPath"]) {
        boat->SetModelPath(node["modelPath"].as<std::string>());
    }

    mPlacementLoader.ApplyScaleFromStageNode(boat.get(), node);

    boat->Initialize();
    if (hasTravelSpeed) {
        boat->SetTravelSpeed(travelSpeed);
    } else if (hasLegacyTravelDuration) {
        // 旧ステージデータは初回保存まで従来の所要時間を維持する。
        boat->SetTravelSpeedFromLegacyDuration(
            legacyTravelDuration);
    }

    Boat* boatPtr = boat.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(boatPtr);
    mGame->AddActor(std::move(boat));
    currentPlanet->AddBoat(boatPtr);

    return boatPtr;
}

void ActorLoadSystem::LoadBoatParts(const char* path)
{
    mActorFactory.LoadActorSequence<BoatParts>(
        path, "boatParts", [](Planet* planet) { planet->RemoveAllBoatParts(); },
        [this](const YAML::Node& node, int index) { return CreateBoatPartsFromStageNode(node, index); });
}

BoatParts* ActorLoadSystem::CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<BoatParts>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, BoatParts* boatParts) {
            planet->AddBoatParts(boatParts);
            planet->Initialize();
        },
        [](BoatParts* boatParts, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            boatParts->ApplyConfig(type);
        });
}

void ActorLoadSystem::LoadKeys(const char* path)
{
    mActorFactory.LoadActorSequence<Key>(
        path, "keys", [](Planet* planet) { planet->RemoveKey(); },
        [this](const YAML::Node& node, int index) { return CreateKeyFromStageNode(node, index); });
}

Key* ActorLoadSystem::CreateKeyFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Key>(
        node, stageYamlIndex, 0.0f, glm::vec3(0.25f), "key.obj",
        [](Planet* planet, Key* key) { planet->SetKey(key); },
        [](Key* key, const YAML::Node&) { key->ApplyConfig(); });
}

void ActorLoadSystem::LoadCrystals(const char* path)
{
    mActorFactory.LoadActorSequence<Crystal>(
        path, "crystals", [](Planet* planet) { planet->RemoveAllCrystals(); },
        [this](const YAML::Node& node, int index) { return CreateCrystalFromStageNode(node, index); });
}

Crystal* ActorLoadSystem::CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Crystal>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.25f), "",
        [](Planet* planet, Crystal* crystal) { planet->AddCrystal(crystal); },
        [](Crystal* crystal, const YAML::Node& node) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";
            crystal->ApplyConfig(type);
        });
}

void ActorLoadSystem::LoadStar(const char* path)
{
    mActorFactory.LoadActorSequence<Star>(
        path, "star", [](Planet* planet) { planet->RemoveStar(); },
        [this](const YAML::Node& node, int index) { return CreateStarFromStageNode(node, index); });
}

Star* ActorLoadSystem::CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<Star>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.0f), "star.obj",
        [](Planet* planet, Star* star) { planet->SetStar(star); },
        [](Star* star, const YAML::Node&) { star->ApplyConfig(); },
        [](Star* star, const YAML::Node& node) {
            if (node["isActive"]) {
                star->SetIsActive(node["isActive"].as<bool>());
            }
        });
}

void ActorLoadSystem::LoadPlatforms(const char* path)
{
    mActorFactory.LoadActorSequence<Platform>(
        path, "platforms", [](Planet* planet) { planet->RemoveAllPlatforms(); },
        [this](const YAML::Node& node, int index) { return CreatePlatformFromStageNode(node, index); });
}

Platform* ActorLoadSystem::CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    Platform* platform = mActorFactory.CreatePlacedActorFromStageNode<Platform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, Platform* platform) { planet->AddPlatform(platform); },
        [stageYamlIndex](Platform* platform, const YAML::Node& node) {
            platform->SetPlatformId(
                node["platformId"]
                    ? node["platformId"].as<std::string>()
                    : "legacy_platforms_" +
                          std::to_string(stageYamlIndex));
            ApplyPlatformMovementConfig(
                platform,
                GetMovementComponentNode(node));
            ApplyPlatformBehaviorConfigs(platform, node);
        });
    if (platform) {
        platform->SetStageSequenceName("platforms");
    }
    return platform;
}

void ActorLoadSystem::LoadLegacyMovingPlatforms(const char* path)
{
    mActorFactory.LoadActorSequence<Platform>(
        path, "movingPlatforms",
        [](Planet* planet) {
            planet->RemovePlatformsByStageSequence("movingPlatforms");
        },
        [this](const YAML::Node& node, int index) {
            return CreateLegacyMovingPlatformFromStageNode(node, index);
        });
}

Platform* ActorLoadSystem::CreateLegacyMovingPlatformFromStageNode(
    const YAML::Node& node,
    int stageYamlIndex)
{
    Platform* platform = mActorFactory.CreatePlacedActorFromStageNode<Platform>(
        node, stageYamlIndex, 1.0f, glm::vec3(3.0f, 0.5f, 3.0f), "platform.obj",
        [](Planet* planet, Platform* platform) { planet->AddPlatform(platform); },
        [stageYamlIndex](Platform* platform, const YAML::Node& node) {
            platform->SetPlatformId(
                node["platformId"]
                    ? node["platformId"].as<std::string>()
                    : "legacy_movingPlatforms_" +
                          std::to_string(stageYamlIndex));
            const YAML::Node componentNode =
                GetMovementComponentNode(node);
            ApplyPlatformMovementConfig(
                platform,
                componentNode ? componentNode : node);
            ApplyPlatformBehaviorConfigs(platform, node);
        });
    if (platform) {
        platform->SetStageSequenceName("movingPlatforms");
    }
    return platform;
}

void ActorLoadSystem::LoadBoatArrivalPoints(const char* path)
{
    mActorFactory.LoadActorSequence<BoatArrivalPoint>(
        path, "boatArrivalPoints", [](Planet* planet) { planet->RemoveAllBoatArrivalPoints(); },
        [this](const YAML::Node& node, int index) { return CreateBoatArrivalPointFromStageNode(node, index); });
}

BoatArrivalPoint* ActorLoadSystem::CreateBoatArrivalPointFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<BoatArrivalPoint>(
        node, stageYamlIndex, 1.0f, glm::vec3(0.4f), "platform.obj",
        [](Planet* planet, BoatArrivalPoint* point) { planet->AddBoatArrivalPoint(point); },
        [](BoatArrivalPoint* point, const YAML::Node&) { point->ApplyConfig(); });
}

void ActorLoadSystem::LoadFallRespawnPoints(const char* path)
{
    mActorFactory.LoadActorSequence<FallRespawnPoint>(
        path, "fallRespawnPoints", [](Planet* planet) { planet->RemoveAllFallRespawnPoints(); },
        [this](const YAML::Node& node, int index) { return CreateFallRespawnPointFromStageNode(node, index); });
}

FallRespawnPoint* ActorLoadSystem::CreateFallRespawnPointFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<FallRespawnPoint>(
        node, stageYamlIndex, 0.0f, glm::vec3(4.0f, 1.0f, 4.0f), "platform.obj",
        [](Planet* planet, FallRespawnPoint* point) { planet->AddFallRespawnPoint(point); },
        [](FallRespawnPoint* point, const YAML::Node&) { point->ApplyConfig(); },
        [](FallRespawnPoint* point, const YAML::Node& node) {
            if (node["damage"]) {
                point->SetDamage(node["damage"].as<float>());
            }
        });
}

void ActorLoadSystem::LoadStageObjects(const char* path)
{
    mActorFactory.LoadActorSequence<StageObject>(
        path, "stageObjects", [](Planet* planet) { planet->RemoveAllStageObjects(); },
        [this](const YAML::Node& node, int index) { return CreateStageObjectFromStageNode(node, index); });
}

StageObject* ActorLoadSystem::CreateStageObjectFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    return mActorFactory.CreatePlacedActorFromStageNode<StageObject>(
        node, stageYamlIndex, 1.0f, glm::vec3(1.0f), "",
        [](Planet* planet, StageObject* stageObject) { planet->AddStageObject(stageObject); },
        [](StageObject* stageObject, const YAML::Node& node) {
            const bool collisionEnabled = node["collision"] ? node["collision"].as<bool>() : true;
            stageObject->SetCollisionEnabled(collisionEnabled);
        });
}

void ActorLoadSystem::ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet,
                                                 int stageYamlIndex, float defaultHeight)
{
    mPlacementLoader.ApplyPlacementFromStageNode(actor, node, currentPlanet, stageYamlIndex, defaultHeight);
}

void ActorLoadSystem::ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node)
{
    mPlacementLoader.ApplyRotationFromStageNode(actor, node);
}

void ActorLoadSystem::ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node)
{
    mPlacementLoader.ApplyScaleFromStageNode(actor, node);
}
