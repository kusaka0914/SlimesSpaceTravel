#include "gfx/debug/stage/StageActorYamlWriter.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/Enemy.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "actor/TutorialTrigger.h"
#include "component/PlatformBehaviorComponents.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include <cmath>
#include <vector>

namespace {
void WritePlatformMovementPath(
    YAML::Node platformNode,
    const std::string& sequenceName,
    const PlatformMovementComponent& movement,
    bool shouldUpdateActorStartPosition)
{
    YAML::Node movementNode =
        sequenceName == "movingPlatforms"
            ? platformNode
            : platformNode["components"]["movement"];

    const glm::vec3 startLocalPos = movement.GetBaseLocalPos();
    const glm::vec3 endLocalPos = movement.GetDestinationLocalPos();
    const glm::vec3 moveOffset = movement.GetMoveOffset();

    movementNode["startLocalPos"][0] = startLocalPos.x;
    movementNode["startLocalPos"][1] = startLocalPos.y;
    movementNode["startLocalPos"][2] = startLocalPos.z;
    movementNode["endLocalPos"][0] = endLocalPos.x;
    movementNode["endLocalPos"][1] = endLocalPos.y;
    movementNode["endLocalPos"][2] = endLocalPos.z;
    movementNode["moveOffset"][0] = moveOffset.x;
    movementNode["moveOffset"][1] = moveOffset.y;
    movementNode["moveOffset"][2] = moveOffset.z;
    movementNode["moveDuration"] = movement.GetMoveDuration();
    movementNode["moveOnPlayer"] = movement.GetMoveOnPlayer();
    movementNode["returnDelay"] = movement.GetReturnDelay();
    movementNode.remove("destinationWaitSeconds");
    movementNode["endpointWaitSeconds"] =
        movement.GetEndpointWaitDurationSeconds();

    if (!shouldUpdateActorStartPosition) {
        return;
    }

    platformNode["pos"][0] = startLocalPos.x;
    platformNode["pos"][1] = startLocalPos.y;
    platformNode["pos"][2] = startLocalPos.z;
}
}

StageActorYamlWriter::StageActorYamlWriter(DebugEditorContext& context)
    : mContext(context)
{
}

void StageActorYamlWriter::SaveAllActorStates()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    StagePlatformConnections::AddStableIdsToLatchedGroupSwitchTargets(
        config);
    WriteAllActorStates(config);
    StageYamlRepository::SaveCurrentStage(mContext, config);
}

void StageActorYamlWriter::SaveEditorAuthoredTransforms()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    StagePlatformConnections::AddStableIdsToLatchedGroupSwitchTargets(
        config);

    std::vector<Actor*> savedActors;
    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(
            mContext.game->GetCurrentStage());
    for (const StageActorInstance& instance : instances) {
        if (!instance.actor ||
            !instance.actor->FindEditorAuthoredTransform() ||
            instance.ref.type == StageActorType::Planet) {
            continue;
        }

        WriteActorState(
            config,
            instance.ref.sequenceName,
            instance.actor,
            ActorTransformWriteMode::EditorAuthoredTransform);
        savedActors.emplace_back(instance.actor);
    }

    for (Player* player : mContext.game->GetPlayers()) {
        if (!player || !player->FindEditorAuthoredTransform()) {
            continue;
        }

        WriteActorState(
            config,
            "players",
            player,
            ActorTransformWriteMode::EditorAuthoredTransform);
        savedActors.emplace_back(player);
    }

    if (savedActors.empty() ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return;
    }

    for (Actor* actor : savedActors) {
        actor->ClearEditorAuthoredTransform();
    }
}

void StageActorYamlWriter::WriteAllActorStates(YAML::Node& config)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::vector<StageActorInstance> actorInstances =
        StageActorQuery::CollectAllActorInstances(
            mContext.game->GetCurrentStage());

    for (const StageActorTypeInfo& actorType : StageActorQuery::GetTypeInfos()) {
        if (actorType.type == StageActorType::Planet) {
            continue;
        }

        for (const StageActorInstance& actorInstance : actorInstances) {
            if (actorInstance.ref.sequenceName != actorType.sequenceName) {
                continue;
            }

            WriteActorState(
                config,
                actorType.sequenceName,
                actorInstance.actor,
                ActorTransformWriteMode::RuntimeState);
        }
    }
}

void StageActorYamlWriter::WriteActorState(
    YAML::Node& config,
    const std::string& sequenceName,
    Actor* actor,
    ActorTransformWriteMode transformWriteMode)
{
    if (!actor) {
        return;
    }

    const int index = actor->GetStageYamlIndex();
    if (index < 0) {
        return;
    }

    const std::size_t yamlIndex = static_cast<std::size_t>(index);

    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        return;
    }

    if (yamlIndex >= config[sequenceName].size()) {
        return;
    }

    const EditorAuthoredTransform* editorTransform =
        transformWriteMode == ActorTransformWriteMode::EditorAuthoredTransform
        ? actor->FindEditorAuthoredTransform()
        : nullptr;
    Planet* authoredPlanet =
        editorTransform && editorTransform->hasPosition
        ? editorTransform->planet
        : actor->GetCurrentPlanet();

    if (editorTransform && editorTransform->hasPosition &&
        !dynamic_cast<const Boat*>(actor)) {
        const int currentPlanetIndex =
            FindPlanetIndex(authoredPlanet);
        if (currentPlanetIndex >= 0) {
            config[sequenceName][yamlIndex]["currentPlanetNum"] =
                currentPlanetIndex;
        }
    }

    if (actor->IsDebugDisabled()) {
        config[sequenceName][yamlIndex]["debugDisabled"] = true;
    } else {
        config[sequenceName][yamlIndex].remove("debugDisabled");
    }

    const int visibleIfStageCleared = actor->GetVisibleIfStageCleared();
    if (visibleIfStageCleared >= 0) {
        config[sequenceName][yamlIndex]["visibleIfStageCleared"] =
            visibleIfStageCleared;
    } else {
        config[sequenceName][yamlIndex].remove("visibleIfStageCleared");
    }

    const int hiddenIfStageCleared = actor->GetHiddenIfStageCleared();
    if (hiddenIfStageCleared >= 0) {
        config[sequenceName][yamlIndex]["hiddenIfStageCleared"] =
            hiddenIfStageCleared;
    } else {
        config[sequenceName][yamlIndex].remove("hiddenIfStageCleared");
    }

    if (actor->ShouldHideWhenRocketAppears()) {
        config[sequenceName][yamlIndex]["hiddenWhenRocketAppears"] =
            true;
    } else {
        config[sequenceName][yamlIndex].remove(
            "hiddenWhenRocketAppears");
    }

    if (actor->ShouldAffectGravityDirection()) {
        config[sequenceName][yamlIndex].remove(
            "affectsGravityDirection");
    } else {
        config[sequenceName][yamlIndex]["affectsGravityDirection"] =
            false;
    }

    if (actor->ShouldReactToOverheadGravityRay()) {
        config[sequenceName][yamlIndex]
              ["reactsToOverheadGravityRay"] = true;
    } else {
        config[sequenceName][yamlIndex].remove(
            "reactsToOverheadGravityRay");
    }

    if (editorTransform && editorTransform->hasPosition) {
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "theta",
            editorTransform->theta);
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "phi",
            editorTransform->phi);
        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "height",
            editorTransform->height);

        glm::vec3 localPosition = editorTransform->localPosition;
        localPosition.x = std::round(localPosition.x * 100.0f) / 100.0f;
        localPosition.y = std::round(localPosition.y * 100.0f) / 100.0f;
        localPosition.z = std::round(localPosition.z * 100.0f) / 100.0f;

        StageYamlRepository::SetSequenceValue(
            config,
            sequenceName,
            yamlIndex,
            "pos",
            YAML::Node(YAML::NodeType::Sequence));
        config[sequenceName][yamlIndex]["pos"][0] = localPosition.x;
        config[sequenceName][yamlIndex]["pos"][1] = localPosition.y;
        config[sequenceName][yamlIndex]["pos"][2] = localPosition.z;

    }

    if (editorTransform && editorTransform->hasRotation) {
        config[sequenceName][yamlIndex]["facingYaw"] =
            editorTransform->facingYaw;
        config[sequenceName][yamlIndex]["rotation"][0] =
            editorTransform->editorRotation.x;
        config[sequenceName][yamlIndex]["rotation"][1] =
            editorTransform->editorRotation.y;
        config[sequenceName][yamlIndex]["rotation"][2] =
            editorTransform->editorRotation.z;

        config[sequenceName][yamlIndex]["rotationQuat"][0] =
            editorTransform->orientation.w;
        config[sequenceName][yamlIndex]["rotationQuat"][1] =
            editorTransform->orientation.x;
        config[sequenceName][yamlIndex]["rotationQuat"][2] =
            editorTransform->orientation.y;
        config[sequenceName][yamlIndex]["rotationQuat"][3] =
            editorTransform->orientation.z;

        config[sequenceName][yamlIndex]["upVec"][0] =
            editorTransform->upDirection.x;
        config[sequenceName][yamlIndex]["upVec"][1] =
            editorTransform->upDirection.y;
        config[sequenceName][yamlIndex]["upVec"][2] =
            editorTransform->upDirection.z;
    }

    if (editorTransform && editorTransform->hasScale) {
        config[sequenceName][yamlIndex]["scale"][0] =
            editorTransform->scale.x;
        config[sequenceName][yamlIndex]["scale"][1] =
            editorTransform->scale.y;
        config[sequenceName][yamlIndex]["scale"][2] =
            editorTransform->scale.z;
    }

    if (transformWriteMode == ActorTransformWriteMode::EditorAuthoredTransform) {
        const Platform* platform =
            dynamic_cast<const Platform*>(actor);
        const PlatformMovementComponent* movement =
            platform
                ? platform->GetMovementComponent()
                : nullptr;
        if (movement && editorTransform &&
            editorTransform->hasPosition) {
            WritePlatformMovementPath(
                config[sequenceName][yamlIndex],
                sequenceName,
                *movement,
                true);
        }
        return;
    }

    YAML::Node actorNode = config[sequenceName][yamlIndex];
    WriteModelAndHazardState(actorNode, actor);
    WritePlatformState(actorNode, sequenceName, actor, editorTransform);
    WriteBoatState(actorNode, actor, editorTransform, authoredPlanet);
    WriteNPCState(actorNode, actor);
    WriteTextureState(actorNode, actor);
}

void StageActorYamlWriter::WriteModelAndHazardState(
    YAML::Node actorNode,
    Actor* actor)
{
    if (dynamic_cast<const Platform*>(actor) ||
        dynamic_cast<const StageObject*>(actor) ||
        dynamic_cast<const BoatArrivalPoint*>(actor) ||
        dynamic_cast<const Star*>(actor) ||
        dynamic_cast<const Enemy*>(actor) ||
        dynamic_cast<const JewelItem*>(actor) ||
        dynamic_cast<const HazardActor*>(actor)) {
        actorNode["modelPath"] = actor->GetModelPath();
    }

    if (const StageObject* stageObject = dynamic_cast<const StageObject*>(actor)) {
        actorNode["collision"] =
            stageObject->GetCollisionEnabled();
    }

    if (const HazardActor* hazardActor =
            dynamic_cast<const HazardActor*>(actor)) {
        actorNode["triggerRadius"] =
            hazardActor->GetTriggerRadius();
        actorNode["damage"] =
            hazardActor->GetDamage();
        actorNode["damageIntervalSeconds"] =
            hazardActor->GetDamageIntervalSeconds();
    }

}

void StageActorYamlWriter::WritePlatformState(
    YAML::Node actorNode,
    const std::string& sequenceName,
    Actor* actor,
    const EditorAuthoredTransform* editorTransform)
{
    if (const Platform* platform = dynamic_cast<const Platform*>(actor)) {
        actorNode["platformId"] =
            platform->GetPlatformId();

        const PlatformMovementComponent* movement =
            platform->GetMovementComponent();

        if (!movement) {
            if (sequenceName != "movingPlatforms" &&
                actorNode["components"]) {
                actorNode["components"].remove("movement");
                if (actorNode["components"].size() == 0) {
                    actorNode.remove("components");
                }
            }
        } else {
            YAML::Node movementNode =
                sequenceName == "movingPlatforms"
                    ? actorNode
                    : actorNode["components"]["movement"];

        const glm::vec3 startLocalPos =
                movement->GetBaseLocalPos();
        const glm::vec3 endLocalPos =
                movement->GetDestinationLocalPos();
            const glm::vec3 moveOffset = movement->GetMoveOffset();

            movementNode["startLocalPos"][0] = startLocalPos.x;
            movementNode["startLocalPos"][1] = startLocalPos.y;
            movementNode["startLocalPos"][2] = startLocalPos.z;
            movementNode["endLocalPos"][0] = endLocalPos.x;
            movementNode["endLocalPos"][1] = endLocalPos.y;
            movementNode["endLocalPos"][2] = endLocalPos.z;
            movementNode["moveOffset"][0] = moveOffset.x;
            movementNode["moveOffset"][1] = moveOffset.y;
            movementNode["moveOffset"][2] = moveOffset.z;
            movementNode["moveDuration"] = movement->GetMoveDuration();
            movementNode["moveOnPlayer"] = movement->GetMoveOnPlayer();
            movementNode["returnDelay"] = movement->GetReturnDelay();
            movementNode.remove("destinationWaitSeconds");
            movementNode["endpointWaitSeconds"] =
                movement->GetEndpointWaitDurationSeconds();

            if (editorTransform && editorTransform->hasPosition) {
                actorNode["pos"][0] = startLocalPos.x;

            // プレビュー中に到着地点へ表示していても、通常の配置位置は
            // 必ず出発地点として保存する。
            actorNode["pos"][0] = startLocalPos.x;
            actorNode["pos"][1] = startLocalPos.y;
            actorNode["pos"][2] = startLocalPos.z;
            }
        }

        YAML::Node platformNode = actorNode;
        const auto removeComponentNode =
            [&platformNode](const char* key) {
                if (platformNode["components"]) {
                    platformNode["components"].remove(key);
                }
            };
        const auto writeVec3 =
            [](YAML::Node node, const char* key, const glm::vec3& value) {
                node[key][0] = value.x;
                node[key][1] = value.y;
                node[key][2] = value.z;
            };

        if (const PlatformFadeOnStandComponent* component =
                platform->GetFadeOnStandComponent()) {
            YAML::Node node =
                platformNode["components"]["fadeOnStand"];
            node["fadeOutDuration"] = component->GetFadeOutDuration();
            node.remove("fadeInDuration");
            node["reappearDelay"] = component->GetReappearDelay();
        } else {
            removeComponentNode("fadeOnStand");
        }

        if (const PlatformJumpToggleComponent* component =
                platform->GetJumpToggleComponent()) {
            YAML::Node node =
                platformNode["components"]["jumpToggle"];
            node["initiallyVisible"] = component->GetInitiallyVisible();
        } else {
            removeComponentNode("jumpToggle");
        }

        if (const PlatformIntervalToggleComponent* component =
                platform->GetIntervalToggleComponent()) {
            YAML::Node node =
                platformNode["components"]["intervalToggle"];
            node["initiallyVisible"] = component->GetInitiallyVisible();
            node["interval"] = component->GetInterval();
            node["warningDuration"] = component->GetWarningDuration();
            node["blinkInterval"] = component->GetBlinkInterval();
        } else {
            removeComponentNode("intervalToggle");
        }

        if (const PlatformDirectionalMovementComponent* component =
                platform->GetDirectionalMovementComponent()) {
            YAML::Node node =
                platformNode["components"]["directionalMovement"];
            node["speed"] = component->GetSpeed();
        } else {
            removeComponentNode("directionalMovement");
        }

        if (const PlatformRotationComponent* component =
                platform->GetRotationComponent()) {
            YAML::Node node =
                platformNode["components"]["rotation"];
            writeVec3(node, "axis", component->GetLocalAxis());
            node["degreesPerSecond"] =
                component->GetDegreesPerSecond();
        } else {
            removeComponentNode("rotation");
        }

        if (const PlatformConveyorComponent* component =
                platform->GetConveyorComponent()) {
            YAML::Node node =
                platformNode["components"]["conveyor"];
            writeVec3(node, "direction", component->GetLocalDirection());
            node["speed"] = component->GetSpeed();
        } else {
            removeComponentNode("conveyor");
        }

        if (platform->GetAdhesionComponent()) {
            platformNode["components"]["adhesion"] =
                YAML::Node(YAML::NodeType::Map);
        } else {
            removeComponentNode("adhesion");
        }

        if (const PlatformPressureSwitchComponent* component =
                platform->GetPressureSwitchComponent()) {
            YAML::Node node =
                platformNode["components"]["pressureSwitch"];
            node["remainsOnAfterPressed"] =
                component->ShouldRemainOnAfterPressed();
            node["inactiveOpacity"] =
                component->GetInactiveOpacity();
            node["targets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const std::string& targetId :
                 component->GetTargetPlatformIds()) {
                node["targets"].push_back(targetId);
            }
            node["enemyTargets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const PlatformRevealTarget& target :
                 component->GetTargetEnemyRefs()) {
                YAML::Node targetNode;
                targetNode["sequence"] = target.sequenceName;
                targetNode["index"] = target.yamlIndex;
                node["enemyTargets"].push_back(targetNode);
            }
            node["hideTargets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const PlatformRevealTarget& target :
                 component->GetHideTargets()) {
                YAML::Node targetNode;
                targetNode["sequence"] = target.sequenceName;
                targetNode["index"] = target.yamlIndex;
                node["hideTargets"].push_back(targetNode);
            }
        } else {
            removeComponentNode("pressureSwitch");
        }

        if (platform->GetEnemyClearUnlockComponent()) {
            platformNode["components"]["enemyClearUnlock"] =
                YAML::Node(YAML::NodeType::Map);
        } else {
            removeComponentNode("enemyClearUnlock");
        }

        if (const PlatformLatchedGroupSwitchComponent* component =
                platform->GetLatchedGroupSwitchComponent()) {
            YAML::Node node =
                platformNode["components"]["latchedGroupSwitch"];
            node["groupId"] = component->GetGroupId();
            node["targets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const PlatformRevealTarget& target :
                 component->GetRevealTargets()) {
                YAML::Node targetNode;
                targetNode["sequence"] = target.sequenceName;
                targetNode["index"] = target.yamlIndex;
                std::string targetPlatformId = target.platformId;
                if (targetPlatformId.empty() &&
                    target.sequenceName == "platforms" &&
                    mContext.game && mContext.game->GetCurrentStage()) {
                    Platform* targetPlatform = dynamic_cast<Platform*>(
                        StageActorQuery::FindActorByRef(
                            mContext.game->GetCurrentStage(),
                            StageActorRef{
                                StageActorType::Platform,
                                target.yamlIndex,
                                target.sequenceName,
                                ""}));
                    if (targetPlatform) {
                        targetPlatformId = targetPlatform->GetPlatformId();
                    }
                }
                if (!targetPlatformId.empty()) {
                    targetNode["platformId"] = targetPlatformId;
                }
                node["targets"].push_back(targetNode);
            }
            node["hideTargets"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const PlatformRevealTarget& target :
                 component->GetHideTargets()) {
                YAML::Node targetNode;
                targetNode["sequence"] = target.sequenceName;
                targetNode["index"] = target.yamlIndex;
                node["hideTargets"].push_back(targetNode);
            }
        } else {
            removeComponentNode("latchedGroupSwitch");
        }

        if (platformNode["components"] &&
            platformNode["components"].size() == 0) {
            platformNode.remove("components");
        }
    }

}

void StageActorYamlWriter::WriteBoatState(
    YAML::Node actorNode,
    Actor* actor,
    const EditorAuthoredTransform* editorTransform,
    Planet* authoredPlanet)
{
    if (const Boat* boat = dynamic_cast<const Boat*>(actor)) {



        const Planet* startPlanet =
            editorTransform && editorTransform->hasPosition
                ? authoredPlanet
                : boat->GetCurrentPlanet();
        actorNode["startPlanet"] =
            FindPlanetIndex(startPlanet);
        actorNode["destPlanet"] =
            FindPlanetIndex(boat->GetDestPlanet());
        actorNode["destStage"] = boat->GetDestStage();
        actorNode["travelSpeed"] =
            boat->GetTravelSpeed();
        actorNode.remove("travelDuration");
        actorNode["destMargin"] =
            boat->GetDestMargin();
        actorNode["launchSequenceId"] =
            boat->GetLaunchSequenceId();
        actorNode["modelPath"] = boat->GetModelPath();

        if (const BoatArrivalPoint* arrivalPoint = boat->GetArrivalPoint()) {
            actorNode["arrivalPointIndex"] =
                arrivalPoint->GetStageYamlIndex();
        } else {
            actorNode.remove("arrivalPointIndex");
        }

        actorNode.remove("currentPlanetNum");
    }

}

void StageActorYamlWriter::WriteNPCState(
    YAML::Node actorNode,
    Actor* actor)
{
    if (const NPC* npc = dynamic_cast<const NPC*>(actor)) {
        actorNode["modelPath"] = npc->GetModelPath();
        const bool isTutorialTrigger =
            dynamic_cast<const TutorialTrigger*>(npc) != nullptr;
        if (isTutorialTrigger) {
            const TutorialTrigger* tutorialTrigger =
                static_cast<const TutorialTrigger*>(npc);
            if (tutorialTrigger->GetTutorialId().empty()) {
                actorNode.remove(
                    "tutorialId");
            } else {
                actorNode["tutorialId"] =
                    tutorialTrigger->GetTutorialId();
            }
            if (tutorialTrigger->GetRequiredCompletedTutorialId().empty()) {
                actorNode.remove(
                    "requiredCompletedTutorialId");
            } else {
                actorNode
                    ["requiredCompletedTutorialId"] =
                    tutorialTrigger->GetRequiredCompletedTutorialId();
            }
            actorNode.remove("name");
            actorNode.remove("radius");
            actorNode.remove(
                "proximityMessage");
        } else {
            actorNode["name"] =
                npc->GetName();
            actorNode["radius"] =
                npc->GetRadius();
            actorNode["forceTalkOnArrival"] =
                npc->GetForcesTalkOnArrival();
        }

        if (isTutorialTrigger ||
            npc->GetProximityMessageMode() ==
            NPCProximityMessageMode::Disabled) {
            actorNode.remove("proximityMessage");
        } else {
            YAML::Node proximityMessage(YAML::NodeType::Map);
            proximityMessage["mode"] =
                npc->GetProximityMessageMode() ==
                        NPCProximityMessageMode::AfterTalk
                    ? "afterTalk"
                    : "always";
            proximityMessage["variants"] =
                YAML::Node(YAML::NodeType::Sequence);
            proximityMessage["rubies"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (std::size_t talkIndex = 0;
                 talkIndex < npc->GetTalkTexts().size();
                 ++talkIndex) {
                const std::string& messageText =
                    npc->GetTalkProximityMessageText(talkIndex);
                if (messageText.empty()) {
                    continue;
                }

                YAML::Node variantNode(YAML::NodeType::Map);
                variantNode["talkIndex"] =
                    static_cast<int>(talkIndex);
                variantNode["text"] = messageText;
                proximityMessage["variants"].push_back(variantNode);

                if (!npc->HasValidTalkProximityMessageRuby(
                        talkIndex)) {
                    continue;
                }

                YAML::Node rubyNode(YAML::NodeType::Map);
                rubyNode["talkIndex"] =
                    static_cast<int>(talkIndex);
                rubyNode["segments"] =
                    YAML::Node(YAML::NodeType::Sequence);
                for (const RubyTextSegment& segment :
                     npc->GetTalkProximityMessageRubySegments(
                         talkIndex)) {
                    YAML::Node segmentNode(YAML::NodeType::Map);
                    segmentNode["text"] = segment.text;
                    segmentNode["ruby"] = segment.showsRuby;
                    if (segment.showsRuby) {
                        segmentNode["reading"] = segment.reading;
                    }
                    rubyNode["segments"].push_back(segmentNode);
                }
                proximityMessage["rubies"].push_back(rubyNode);
            }
            if (proximityMessage["rubies"].size() == 0) {
                proximityMessage.remove("rubies");
            }
            proximityMessage["range"] =
                npc->GetProximityMessageRange();
            proximityMessage["height"] =
                npc->GetProximityMessageHeight();
            proximityMessage["scale"] =
                npc->GetProximityMessageScale();
            actorNode["proximityMessage"] =
                proximityMessage;
        }

        actorNode["talkTexts"] =
            YAML::Node(YAML::NodeType::Sequence);
        for (const std::string& talkText : npc->GetTalkTexts()) {
            actorNode["talkTexts"].push_back(talkText);
        }

        YAML::Node talkStageClearConditions(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const int stageCondition =
                npc->GetTalkStageClearCondition(talkIndex);
            if (stageCondition < 0) {
                continue;
            }

            YAML::Node conditionNode(YAML::NodeType::Map);
            conditionNode["talkIndex"] = static_cast<int>(talkIndex);
            conditionNode["stage"] = stageCondition;
            talkStageClearConditions.push_back(conditionNode);
        }
        if (talkStageClearConditions.size() > 0) {
            actorNode["talkStageClearConditions"] =
                talkStageClearConditions;
        } else {
            actorNode.remove(
                "talkStageClearConditions");
        }

        YAML::Node talkCameraFocus(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const NPCTalkCameraFocusTarget* focusTarget =
                npc->GetTalkCameraFocusTarget(talkIndex);
            if (!focusTarget || !focusTarget->IsValid()) {
                continue;
            }

            YAML::Node focusNode(YAML::NodeType::Map);
            focusNode["talkIndex"] = static_cast<int>(talkIndex);
            focusNode["sequence"] = focusTarget->sequenceName;
            focusNode["index"] = focusTarget->yamlIndex;
            talkCameraFocus.push_back(focusNode);
        }

        if (talkCameraFocus.size() > 0) {
            actorNode["talkCameraFocus"] = talkCameraFocus;
        } else {
            actorNode.remove("talkCameraFocus");
        }

        YAML::Node talkAdvanceConditions(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            const TalkPageAdvanceCondition condition =
                npc->GetTalkAdvanceCondition(talkIndex);
            if (condition == TalkPageAdvanceCondition::Confirm) {
                continue;
            }

            YAML::Node conditionNode(YAML::NodeType::Map);
            conditionNode["talkIndex"] =
                static_cast<int>(talkIndex);
            conditionNode["condition"] =
                GetTalkPageAdvanceConditionId(condition);
            talkAdvanceConditions.push_back(conditionNode);
        }

        if (talkAdvanceConditions.size() > 0) {
            actorNode["talkAdvanceConditions"] =
                talkAdvanceConditions;
        } else {
            actorNode.remove(
                "talkAdvanceConditions");
        }

        YAML::Node talkOpeningAfterPages(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            if (!npc->GetTalkStartsOpeningAfterPage(talkIndex)) {
                continue;
            }

            YAML::Node pageNode(YAML::NodeType::Map);
            pageNode["talkIndex"] = static_cast<int>(talkIndex);
            talkOpeningAfterPages.push_back(pageNode);
        }
        if (talkOpeningAfterPages.size() > 0) {
            actorNode["talkOpeningAfterPages"] =
                talkOpeningAfterPages;
        } else {
            actorNode.remove(
                "talkOpeningAfterPages");
        }

        YAML::Node talkEndingAfterPages(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            if (!npc->GetTalkStartsEndingAfterPage(talkIndex)) {
                continue;
            }
            YAML::Node pageNode(YAML::NodeType::Map);
            pageNode["talkIndex"] = static_cast<int>(talkIndex);
            talkEndingAfterPages.push_back(pageNode);
        }
        if (talkEndingAfterPages.size() > 0) {
            actorNode["talkEndingAfterPages"] =
                talkEndingAfterPages;
        } else {
            actorNode.remove(
                "talkEndingAfterPages");
        }

        YAML::Node talkRubies(YAML::NodeType::Sequence);
        for (std::size_t talkIndex = 0;
             talkIndex < npc->GetTalkTexts().size();
             ++talkIndex) {
            if (!npc->HasValidTalkRuby(talkIndex)) {
                continue;
            }

            YAML::Node rubyNode(YAML::NodeType::Map);
            rubyNode["talkIndex"] = static_cast<int>(talkIndex);
            rubyNode["segments"] = YAML::Node(YAML::NodeType::Sequence);

            for (const RubyTextSegment& segment :
                 npc->GetTalkRubySegments(talkIndex)) {
                YAML::Node segmentNode(YAML::NodeType::Map);
                segmentNode["text"] = segment.text;
                segmentNode["ruby"] = segment.showsRuby;
                if (segment.showsRuby) {
                    segmentNode["reading"] = segment.reading;
                }
                rubyNode["segments"].push_back(segmentNode);
            }

            talkRubies.push_back(rubyNode);
        }

        if (talkRubies.size() > 0) {
            actorNode["talkRubies"] = talkRubies;
        } else {
            actorNode.remove("talkRubies");
        }
    }

}

void StageActorYamlWriter::WriteTextureState(
    YAML::Node actorNode,
    Actor* actor)
{
    const bool supportsTextureTiling =
        dynamic_cast<const Platform*>(actor) ||
        dynamic_cast<const StageObject*>(actor) ||
        dynamic_cast<const Boat*>(actor) ||
        dynamic_cast<const BoatArrivalPoint*>(actor) ||
        dynamic_cast<const JewelItem*>(actor) ||
        dynamic_cast<const HazardActor*>(actor);
    if (supportsTextureTiling) {
        const glm::vec2 textureTiling = actor->GetTextureTiling();
        actorNode["textureTiling"][0] = textureTiling.x;
        actorNode["textureTiling"][1] = textureTiling.y;
    }

    const std::string& textureOverride = actor->GetTextureOverridePath();
    if (textureOverride.empty()) {
        actorNode.remove("textureOverride");
    } else {
        actorNode["textureOverride"] = textureOverride;
    }
}

int StageActorYamlWriter::FindPlanetIndex(
    const Planet* targetPlanet) const
{
    Stage* stage = mContext.game
        ? mContext.game->GetCurrentStage()
        : nullptr;
    if (!stage) {
        return -1;
    }

    const std::vector<Planet*>& planets = stage->GetPlanets();
    for (std::size_t planetIndex = 0;
         planetIndex < planets.size();
         ++planetIndex) {
        if (planets[planetIndex] == targetPlanet) {
            return static_cast<int>(planetIndex);
        }
    }

    return -1;
}
