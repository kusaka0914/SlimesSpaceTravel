#include "gfx/debug/stage/StageEditCommandController.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCWorkMetadata.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace {
std::string CreateUniquePlatformId(const YAML::Node& config)
{
    std::unordered_set<std::string> usedIds;
    if (config && config.IsMap()) {
        for (const auto& entry : config) {
            const YAML::Node sequence = entry.second;
            if (!sequence || !sequence.IsSequence()) {
                continue;
            }
            for (const YAML::Node& node : sequence) {
                if (!node || !node.IsMap()) {
                    continue;
                }
                if (node["platformId"]) {
                    usedIds.insert(node["platformId"].as<std::string>());
                }

                const YAML::Node components = node["components"];
                if (!components || !components.IsMap()) {
                    continue;
                }

                const YAML::Node pressureSwitch =
                    components["pressureSwitch"];
                if (!pressureSwitch || !pressureSwitch.IsMap()) {
                    continue;
                }

                const YAML::Node targets = pressureSwitch["targets"];
                if (targets && targets.IsSequence()) {
                    for (const YAML::Node& target : targets) {
                        if (target && target.IsScalar()) {
                            usedIds.insert(target.as<std::string>());
                        }
                    }
                }
            }
        }
    }

    for (int suffix = 1;; ++suffix) {
        const std::string candidate =
            "platform_" + std::to_string(suffix);
        if (!usedIds.contains(candidate)) {
            return candidate;
        }
    }
}

bool TryReadPlanetIndex(
    const YAML::Node& node,
    const char* key,
    int defaultIndex,
    int& outPlanetIndex)
{
    if (!node[key]) {
        outPlanetIndex = defaultIndex;
        return true;
    }

    try {
        outPlanetIndex = node[key].as<int>();
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

void RenumberPlanetNodes(YAML::Node planetNodes)
{
    if (!planetNodes || !planetNodes.IsSequence()) {
        return;
    }

    for (std::size_t planetIndex = 0;
         planetIndex < planetNodes.size();
         ++planetIndex) {
        if (planetNodes[planetIndex].IsMap()) {
            planetNodes[planetIndex]["stageNum"] =
                static_cast<int>(planetIndex);
        }
    }
}

glm::vec3 ReadPlanetCenter(const YAML::Node& planetNode)
{
    const YAML::Node center = planetNode["center"];
    if (!center || !center.IsSequence() || center.size() < 3) {
        return glm::vec3(0.0f);
    }
    return glm::vec3(
        center[0].as<float>(0.0f),
        center[1].as<float>(0.0f),
        center[2].as<float>(0.0f));
}

float ReadPlanetRadius(const YAML::Node& planetNode)
{
    const YAML::Node scale = planetNode["scale"];
    if (!scale || !scale.IsSequence() || scale.size() == 0) {
        return 1.0f;
    }
    return std::abs(scale[0].as<float>(1.0f));
}

bool TryReadVec3(
    const YAML::Node& node,
    const char* key,
    glm::vec3& outVector)
{
    const YAML::Node vectorNode = node[key];
    if (!vectorNode || !vectorNode.IsSequence() || vectorNode.size() < 3) {
        return false;
    }
    outVector = glm::vec3(
        vectorNode[0].as<float>(),
        vectorNode[1].as<float>(),
        vectorNode[2].as<float>());
    return true;
}

void WriteVec3(
    YAML::Node node,
    const char* key,
    const glm::vec3& vector)
{
    node[key][0] = vector.x;
    node[key][1] = vector.y;
    node[key][2] = vector.z;
}

void TranslateMovementPath(
    YAML::Node movementNode,
    const glm::vec3& localPositionOffset)
{
    if (!movementNode || !movementNode.IsMap()) {
        return;
    }

    constexpr const char* pathPositionKeys[] = {
        "startLocalPos", "endLocalPos"};
    for (const char* pathPositionKey : pathPositionKeys) {
        glm::vec3 localPosition;
        if (TryReadVec3(movementNode, pathPositionKey, localPosition)) {
            WriteVec3(
                movementNode,
                pathPositionKey,
                localPosition + localPositionOffset);
        }
    }
}

void PreserveActorWorldPositionAfterPlanetReassignment(
    YAML::Node actorNode,
    float previousPlanetRadius,
    float replacementPlanetRadius,
    const glm::vec3& localPositionOffset)
{
    glm::vec3 previousLocalPosition;
    if (!TryReadVec3(actorNode, "pos", previousLocalPosition)) {
        const float theta = actorNode["theta"].as<float>(0.0f);
        const float phi = actorNode["phi"].as<float>(0.0f);
        const float height = actorNode["height"].as<float>(0.0f);
        const glm::vec3 direction(
            std::cos(phi) * std::cos(theta),
            std::sin(phi),
            std::cos(phi) * std::sin(theta));
        previousLocalPosition =
            direction * (previousPlanetRadius + height);
    }

    const glm::vec3 replacementLocalPosition =
        previousLocalPosition + localPositionOffset;
    WriteVec3(actorNode, "pos", replacementLocalPosition);

    const float distanceFromReplacementCenter =
        glm::length(replacementLocalPosition);
    if (distanceFromReplacementCenter > 0.000001f) {
        const glm::vec3 direction =
            replacementLocalPosition / distanceFromReplacementCenter;
        actorNode["theta"] = std::atan2(direction.z, direction.x);
        actorNode["phi"] = std::asin(
            glm::clamp(direction.y, -1.0f, 1.0f));
        actorNode["height"] =
            distanceFromReplacementCenter - replacementPlanetRadius;
    }

    TranslateMovementPath(actorNode, localPositionOffset);
    const YAML::Node readOnlyActorNode = actorNode;
    const YAML::Node components = readOnlyActorNode["components"];
    if (components && components.IsMap() && components["movement"]) {
        TranslateMovementPath(
            actorNode["components"]["movement"],
            localPositionOffset);
    }
}

void ReassignPlayersFromDeletedPlanet(
    YAML::Node& stageYaml,
    int deletedPlanetIndex,
    int replacementPlanetIndex)
{
    YAML::Node players = stageYaml["players"];
    if (!players || !players.IsSequence()) {
        return;
    }

    for (YAML::Node playerNode : players) {
        if (!playerNode.IsMap()) {
            continue;
        }

        int currentPlanetIndex = 0;
        if (!TryReadPlanetIndex(
                playerNode,
                "currentPlanetNum",
                0,
                currentPlanetIndex)) {
            continue;
        }

        if (currentPlanetIndex == deletedPlanetIndex) {
            playerNode["currentPlanetNum"] = replacementPlanetIndex;
        } else if (currentPlanetIndex > deletedPlanetIndex) {
            playerNode["currentPlanetNum"] = currentPlanetIndex - 1;
        }
    }
}

void ReassignPlayersPreservingWorldPositions(
    YAML::Node& stageYaml,
    int deletedPlanetIndex,
    int replacementPlanetIndex,
    float deletedPlanetRadius,
    float replacementPlanetRadius,
    const glm::vec3& localPositionOffset)
{
    YAML::Node players = stageYaml["players"];
    if (!players || !players.IsSequence()) {
        return;
    }

    for (YAML::Node playerNode : players) {
        if (!playerNode.IsMap()) {
            continue;
        }

        int currentPlanetIndex = 0;
        if (!TryReadPlanetIndex(
                playerNode,
                "currentPlanetNum",
                0,
                currentPlanetIndex)) {
            continue;
        }

        if (currentPlanetIndex == deletedPlanetIndex) {
            PreserveActorWorldPositionAfterPlanetReassignment(
                playerNode,
                deletedPlanetRadius,
                replacementPlanetRadius,
                localPositionOffset);
            playerNode["currentPlanetNum"] =
                replacementPlanetIndex;
        } else if (currentPlanetIndex > deletedPlanetIndex) {
            playerNode["currentPlanetNum"] =
                currentPlanetIndex - 1;
        }
    }
}

int ResolvePlanetIndexAfterDeletion(
    int sourcePlanetIndex,
    int deletedPlanetIndex,
    int replacementPlanetIndex)
{
    if (sourcePlanetIndex == deletedPlanetIndex) {
        return replacementPlanetIndex;
    }
    if (sourcePlanetIndex > deletedPlanetIndex) {
        return sourcePlanetIndex - 1;
    }
    return sourcePlanetIndex;
}

void ReassignActorsPreservingWorldPositions(
    YAML::Node& stageYaml,
    const std::string& sequenceName,
    int deletedPlanetIndex,
    int replacementPlanetIndex,
    float deletedPlanetRadius,
    float replacementPlanetRadius,
    const glm::vec3& localPositionOffset)
{
    YAML::Node actorNodes = stageYaml[sequenceName];
    if (!actorNodes || !actorNodes.IsSequence()) {
        return;
    }

    for (YAML::Node actorNode : actorNodes) {
        if (!actorNode.IsMap()) {
            continue;
        }

        int currentPlanetIndex = 0;
        if (!TryReadPlanetIndex(
                actorNode,
                "currentPlanetNum",
                0,
                currentPlanetIndex)) {
            continue;
        }

        if (currentPlanetIndex == deletedPlanetIndex) {
            PreserveActorWorldPositionAfterPlanetReassignment(
                actorNode,
                deletedPlanetRadius,
                replacementPlanetRadius,
                localPositionOffset);
        }
        actorNode["currentPlanetNum"] = ResolvePlanetIndexAfterDeletion(
            currentPlanetIndex,
            deletedPlanetIndex,
            replacementPlanetIndex);
    }
}

void ReassignBoatsPreservingWorldPositions(
    YAML::Node& stageYaml,
    int deletedPlanetIndex,
    int replacementPlanetIndex,
    float deletedPlanetRadius,
    float replacementPlanetRadius,
    const glm::vec3& localPositionOffset)
{
    YAML::Node boats = stageYaml["boats"];
    if (!boats || !boats.IsSequence()) {
        return;
    }

    for (YAML::Node boatNode : boats) {
        if (!boatNode.IsMap()) {
            continue;
        }

        int startPlanetIndex = 0;
        if (TryReadPlanetIndex(
                boatNode,
                "startPlanet",
                0,
                startPlanetIndex)) {
            if (startPlanetIndex == deletedPlanetIndex) {
                PreserveActorWorldPositionAfterPlanetReassignment(
                    boatNode,
                    deletedPlanetRadius,
                    replacementPlanetRadius,
                    localPositionOffset);
            }
            boatNode["startPlanet"] = ResolvePlanetIndexAfterDeletion(
                startPlanetIndex,
                deletedPlanetIndex,
                replacementPlanetIndex);
        }

        int destinationPlanetIndex = 0;
        if (TryReadPlanetIndex(
                boatNode,
                "destPlanet",
                0,
                destinationPlanetIndex)) {
            boatNode["destPlanet"] = ResolvePlanetIndexAfterDeletion(
                destinationPlanetIndex,
                deletedPlanetIndex,
                replacementPlanetIndex);
        }
    }
}

void ReassignUGCPlatformCellsFromDeletedPlanet(
    YAML::Node& stageYaml,
    int deletedPlanetIndex,
    int replacementPlanetIndex)
{
    YAML::Node cells = stageYaml["ugcPlatformCells"];
    if (!cells || !cells.IsSequence()) {
        return;
    }

    for (YAML::Node cell : cells) {
        if (!cell.IsMap() || !cell["planetIndex"]) {
            continue;
        }

        cell["planetIndex"] = ResolvePlanetIndexAfterDeletion(
            cell["planetIndex"].as<int>(0),
            deletedPlanetIndex,
            replacementPlanetIndex);
    }
}

void RemoveBoatsReferencingDeletedPlanet(
    YAML::Node& stageYaml,
    int deletedPlanetIndex)
{
    const YAML::Node boats = stageYaml["boats"];
    if (!boats || !boats.IsSequence()) {
        return;
    }

    YAML::Node remainingBoats(YAML::NodeType::Sequence);
    for (const YAML::Node& sourceBoatNode : boats) {
        if (!sourceBoatNode.IsMap()) {
            remainingBoats.push_back(sourceBoatNode);
            continue;
        }

        int startPlanetIndex = 0;
        int destinationPlanetIndex = 0;
        const bool hasValidStartPlanet =
            TryReadPlanetIndex(
                sourceBoatNode,
                "startPlanet",
                0,
                startPlanetIndex);
        const bool hasValidDestinationPlanet =
            TryReadPlanetIndex(
                sourceBoatNode,
                "destPlanet",
                0,
                destinationPlanetIndex);
        if (!hasValidStartPlanet ||
            !hasValidDestinationPlanet) {
            remainingBoats.push_back(sourceBoatNode);
            continue;
        }

        const bool referencesDeletedPlanet =
            startPlanetIndex == deletedPlanetIndex ||
            destinationPlanetIndex == deletedPlanetIndex;
        if (referencesDeletedPlanet) {
            continue;
        }

        YAML::Node boatNode = YAML::Clone(sourceBoatNode);
        if (startPlanetIndex > deletedPlanetIndex) {
            boatNode["startPlanet"] = startPlanetIndex - 1;
        }
        if (destinationPlanetIndex > deletedPlanetIndex) {
            boatNode["destPlanet"] =
                destinationPlanetIndex - 1;
        }
        remainingBoats.push_back(boatNode);
    }

    stageYaml["boats"] = remainingBoats;
}

void RemoveActorsOnDeletedPlanet(
    YAML::Node& stageYaml,
    const std::string& sequenceName,
    int deletedPlanetIndex)
{
    const YAML::Node actorNodes = stageYaml[sequenceName];
    if (!actorNodes || !actorNodes.IsSequence()) {
        return;
    }

    YAML::Node remainingActors(YAML::NodeType::Sequence);
    for (const YAML::Node& sourceActorNode : actorNodes) {
        if (!sourceActorNode.IsMap()) {
            remainingActors.push_back(sourceActorNode);
            continue;
        }

        int currentPlanetIndex = 0;
        if (!TryReadPlanetIndex(
                sourceActorNode,
                "currentPlanetNum",
                0,
                currentPlanetIndex)) {
            remainingActors.push_back(sourceActorNode);
            continue;
        }

        if (currentPlanetIndex == deletedPlanetIndex) {
            continue;
        }

        YAML::Node actorNode = YAML::Clone(sourceActorNode);
        if (currentPlanetIndex > deletedPlanetIndex) {
            actorNode["currentPlanetNum"] =
                currentPlanetIndex - 1;
        }
        remainingActors.push_back(actorNode);
    }

    stageYaml[sequenceName] = remainingActors;
}

void RemoveUGCPlatformCellsOnDeletedPlanet(
    YAML::Node& stageYaml,
    int deletedPlanetIndex)
{
    const YAML::Node cells = stageYaml["ugcPlatformCells"];
    if (!cells || !cells.IsSequence()) {
        return;
    }

    YAML::Node remainingCells(YAML::NodeType::Sequence);
    for (const YAML::Node& sourceCell : cells) {
        if (!sourceCell.IsMap() || !sourceCell["planetIndex"]) {
            remainingCells.push_back(YAML::Clone(sourceCell));
            continue;
        }
        const int planetIndex = sourceCell["planetIndex"].as<int>(0);
        if (planetIndex == deletedPlanetIndex) {
            continue;
        }
        YAML::Node cell = YAML::Clone(sourceCell);
        if (planetIndex > deletedPlanetIndex) {
            cell["planetIndex"] = planetIndex - 1;
        }
        remainingCells.push_back(cell);
    }
    stageYaml["ugcPlatformCells"] = remainingCells;
}

bool IsDeletedActorIndex(
    const std::vector<int>& deletedIndices,
    int actorIndex)
{
    return std::binary_search(
        deletedIndices.begin(),
        deletedIndices.end(),
        actorIndex);
}

int CalculateActorIndexAfterDeletion(
    const std::vector<int>& deletedIndices,
    int actorIndex)
{
    const auto firstDeletedIndexAfterActor = std::lower_bound(
        deletedIndices.begin(),
        deletedIndices.end(),
        actorIndex);
    return actorIndex - static_cast<int>(
        std::distance(
            deletedIndices.begin(),
            firstDeletedIndexAfterActor));
}

void UpdateRevealTargetReferencesAfterActorDeletion(
    YAML::Node targetNodes,
    const std::unordered_map<std::string, std::vector<int>>&
        deletedIndicesBySequence)
{
    if (!targetNodes || !targetNodes.IsSequence()) {
        return;
    }

    YAML::Node remainingTargetNodes(YAML::NodeType::Sequence);
    for (const YAML::Node& sourceTargetNode : targetNodes) {
        if (!sourceTargetNode.IsMap() ||
            !sourceTargetNode["sequence"] ||
            !sourceTargetNode["index"]) {
            remainingTargetNodes.push_back(sourceTargetNode);
            continue;
        }

        std::string targetSequenceName;
        int targetActorIndex = -1;
        try {
            targetSequenceName =
                sourceTargetNode["sequence"].as<std::string>();
            targetActorIndex = sourceTargetNode["index"].as<int>();
        } catch (const YAML::Exception&) {
            remainingTargetNodes.push_back(sourceTargetNode);
            continue;
        }

        const auto deletedIndices =
            deletedIndicesBySequence.find(targetSequenceName);
        if (deletedIndices == deletedIndicesBySequence.end()) {
            remainingTargetNodes.push_back(sourceTargetNode);
            continue;
        }

        if (IsDeletedActorIndex(
                deletedIndices->second,
                targetActorIndex)) {
            continue;
        }

        YAML::Node targetNode = YAML::Clone(sourceTargetNode);
        targetNode["index"] = CalculateActorIndexAfterDeletion(
            deletedIndices->second,
            targetActorIndex);
        remainingTargetNodes.push_back(targetNode);
    }

    targetNodes = remainingTargetNodes;
}

void UpdateSwitchTargetReferencesAfterActorDeletion(
    YAML::Node& stageYaml,
    const std::unordered_map<std::string, std::vector<int>>&
        deletedIndicesBySequence)
{
    if (!stageYaml || !stageYaml.IsMap()) {
        return;
    }

    for (const auto& entry : stageYaml) {
        YAML::Node actorNodes = entry.second;
        if (!actorNodes || !actorNodes.IsSequence()) {
            continue;
        }

        for (YAML::Node actorNode : actorNodes) {
            YAML::Node components = actorNode["components"];
            if (!components || !components.IsMap()) {
                continue;
            }

            YAML::Node pressureSwitch = components["pressureSwitch"];
            if (pressureSwitch && pressureSwitch.IsMap()) {
                UpdateRevealTargetReferencesAfterActorDeletion(
                    pressureSwitch["enemyTargets"],
                    deletedIndicesBySequence);
                UpdateRevealTargetReferencesAfterActorDeletion(
                    pressureSwitch["hideTargets"],
                    deletedIndicesBySequence);
            }

            YAML::Node latchedGroupSwitch =
                components["latchedGroupSwitch"];
            if (latchedGroupSwitch && latchedGroupSwitch.IsMap()) {
                UpdateRevealTargetReferencesAfterActorDeletion(
                    latchedGroupSwitch["targets"],
                    deletedIndicesBySequence);
                UpdateRevealTargetReferencesAfterActorDeletion(
                    latchedGroupSwitch["hideTargets"],
                    deletedIndicesBySequence);
            }
        }
    }
}
}

StageEditCommandController::StageEditCommandController(DebugEditorContext& context,
                                                       StageSelectionController& selectionController)
    : mContext(context),
      mSelectionController(selectionController)
{
}

bool StageEditCommandController::ConsumeRequestOpenPlacement()
{
    const bool result = mRequestOpenPlacement;
    mRequestOpenPlacement = false;
    return result;
}

void StageEditCommandController::PushUndo()
{
    if (!mContext.game) {
        return;
    }

    std::string yamlText;

    if (!StageYamlRepository::ReadCurrentStageText(mContext, yamlText)) {
        return;
    }

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Skip pushing invalid undo yaml: " << e.what() << std::endl;
        return;
    }

    mEditHistory.PushUndoSnapshot(yamlText);

    if (mContext.game->GetIsUGCMode()) {
        YAML::Node stageYaml;
        if (StageYamlRepository::LoadCurrentStage(
                mContext, stageYaml)) {
            UGCWorkMetadata::InvalidateClearVerification(stageYaml);
            StageYamlRepository::SaveCurrentStage(
                mContext, stageYaml);
        }
    }

}

bool StageEditCommandController::RestoreUndo()
{
    const std::string* yamlText = mEditHistory.FindUndoSnapshot();
    if (!mContext.game || !yamlText) {
        return false;
    }

    std::string currentYamlText;
    if (!StageYamlRepository::ReadCurrentStageText(
            mContext, currentYamlText)) {
        return false;
    }
    if (!StageYamlRepository::WriteCurrentStageTextAtomically(
            mContext, *yamlText)) {
        return false;
    }

    mEditHistory.CommitUndo(currentYamlText);

    mSelectionController.Clear();

    mContext.game->ReloadCurrentStage();

    return true;
}

bool StageEditCommandController::DeleteSelectedKeys(const std::unordered_set<std::string>& selectedKeys)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    if (selectedKeys.empty()) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    std::vector<StageActorRef> targets = StageActorQuery::CollectAllTargets(mContext.game->GetCurrentStage());

    std::unordered_map<std::string, std::vector<int>> deleteIndicesBySequence;
    for (const StageActorRef& target : targets) {
        const std::string key = StageActorQuery::MakeKey(target);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        deleteIndicesBySequence[target.sequenceName].push_back(target.yamlIndex);
    }

    if (deleteIndicesBySequence.empty()) {
        return false;
    }

    PushUndo();

    for (auto& deletionEntry : deleteIndicesBySequence) {
        std::vector<int>& indices = deletionEntry.second;
        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    }



    UpdateSwitchTargetReferencesAfterActorDeletion(
        config,
        deleteIndicesBySequence);

    for (auto& deletionEntry : deleteIndicesBySequence) {
        const std::string& sequenceName = deletionEntry.first;
        std::vector<int>& indices = deletionEntry.second;

        std::sort(indices.rbegin(), indices.rend());

        for (int index : indices) {
            StageYamlRepository::RemoveSequenceElement(config, sequenceName, index);
        }
    }

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mSelectionController.Clear();

    mContext.game->ReloadCurrentStage();

    return true;
}

void StageEditCommandController::ClearHistory()
{
    mEditHistory.Clear();
}

bool StageEditCommandController::RestoreRedo()
{
    const std::string* yamlText = mEditHistory.FindRedoSnapshot();
    if (!mContext.game || !yamlText) {
        return false;
    }

    std::string currentYamlText;
    if (!StageYamlRepository::ReadCurrentStageText(
            mContext, currentYamlText)) {
        return false;
    }
    if (!StageYamlRepository::WriteCurrentStageTextAtomically(
            mContext, *yamlText)) {
        return false;
    }

    mEditHistory.CommitRedo(currentYamlText);
    mSelectionController.Clear();
    mContext.game->ReloadCurrentStage();
    return true;
}

bool StageEditCommandController::DeletePlanet(int planetIndex)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext,
            stageYaml)) {
        return false;
    }

    const YAML::Node planets = stageYaml["planets"];
    if (!planets || !planets.IsSequence() ||
        planets.size() <= 1 ||
        planetIndex < 0 ||
        planetIndex >= static_cast<int>(planets.size())) {
        return false;
    }

    PushUndo();

    if (!StageYamlRepository::RemoveSequenceElement(
            stageYaml,
            "planets",
            planetIndex)) {
        return false;
    }

    RenumberPlanetNodes(stageYaml["planets"]);

    const int remainingPlanetCount =
        static_cast<int>(stageYaml["planets"].size());
    const int replacementPlanetIndex =
        std::min(planetIndex, remainingPlanetCount - 1);
    ReassignPlayersFromDeletedPlanet(
        stageYaml,
        planetIndex,
        replacementPlanetIndex);
    RemoveBoatsReferencingDeletedPlanet(
        stageYaml,
        planetIndex);

    std::unordered_set<std::string> actorSequenceNames;
    for (const StageActorTypeInfo& typeInfo :
         StageActorQuery::GetTypeInfos()) {



        if (typeInfo.sequenceName != "boats" &&
            typeInfo.sequenceName != "planets") {
            actorSequenceNames.insert(typeInfo.sequenceName);
        }
    }
    for (const std::string& sequenceName :
         actorSequenceNames) {
        RemoveActorsOnDeletedPlanet(
            stageYaml,
            sequenceName,
            planetIndex);
    }
    RemoveUGCPlatformCellsOnDeletedPlanet(
        stageYaml,
        planetIndex);

    if (!StageYamlRepository::SaveCurrentStage(
            mContext,
            stageYaml,
            false)) {
        return false;
    }

    mSelectionController.Clear();
    mContext.game->ReloadCurrentStage();
    return true;
}

bool StageEditCommandController::DeletePlanetOnly(int planetIndex)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return false;
    }

    const YAML::Node planets = stageYaml["planets"];
    if (!planets || !planets.IsSequence() ||
        planets.size() <= 1 ||
        planetIndex < 0 ||
        planetIndex >= static_cast<int>(planets.size())) {
        return false;
    }

    const YAML::Node deletedPlanetNode = YAML::Clone(planets[planetIndex]);
    const int replacementPlanetSourceIndex =
        planetIndex + 1 < static_cast<int>(planets.size())
            ? planetIndex + 1
            : planetIndex - 1;
    const YAML::Node replacementPlanetNode =
        YAML::Clone(planets[replacementPlanetSourceIndex]);
    const glm::vec3 localPositionOffset =
        ReadPlanetCenter(deletedPlanetNode) -
        ReadPlanetCenter(replacementPlanetNode);
    const float deletedPlanetRadius =
        ReadPlanetRadius(deletedPlanetNode);
    const float replacementPlanetRadius =
        ReadPlanetRadius(replacementPlanetNode);

    PushUndo();
    if (!StageYamlRepository::RemoveSequenceElement(
            stageYaml,
            "planets",
            planetIndex)) {
        return false;
    }

    RenumberPlanetNodes(stageYaml["planets"]);
    const int remainingPlanetCount =
        static_cast<int>(stageYaml["planets"].size());
    const int replacementPlanetIndex = std::min(
        planetIndex,
        remainingPlanetCount - 1);

    ReassignPlayersPreservingWorldPositions(
        stageYaml,
        planetIndex,
        replacementPlanetIndex,
        deletedPlanetRadius,
        replacementPlanetRadius,
        localPositionOffset);
    ReassignBoatsPreservingWorldPositions(
        stageYaml,
        planetIndex,
        replacementPlanetIndex,
        deletedPlanetRadius,
        replacementPlanetRadius,
        localPositionOffset);
    std::unordered_set<std::string> actorSequenceNames;
    for (const StageActorTypeInfo& typeInfo :
         StageActorQuery::GetTypeInfos()) {
        if (typeInfo.sequenceName == "planets" ||
            typeInfo.sequenceName == "boats") {
            continue;
        }

        actorSequenceNames.insert(typeInfo.sequenceName);
    }
    for (const std::string& sequenceName : actorSequenceNames) {
        ReassignActorsPreservingWorldPositions(
            stageYaml,
            sequenceName,
            planetIndex,
            replacementPlanetIndex,
            deletedPlanetRadius,
            replacementPlanetRadius,
            localPositionOffset);
    }
    ReassignUGCPlatformCellsFromDeletedPlanet(
        stageYaml,
        planetIndex,
        replacementPlanetIndex);

    if (!StageYamlRepository::SaveCurrentStage(
            mContext,
            stageYaml,
            false)) {
        return false;
    }

    mSelectionController.Clear();
    mContext.game->ReloadCurrentStage();
    return true;
}

bool StageEditCommandController::DuplicateSelectedKeys(const std::unordered_set<std::string>& selectedKeys)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    if (selectedKeys.empty()) {
        return false;
    }

    YAML::Node stageYaml;

    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return false;
    }

    const std::vector<StageActorRef> targets =
        StageActorQuery::CollectAllTargets(
            mContext.game->GetCurrentStage(),
            true);

    std::unordered_set<std::string> newSelectedKeys;
    bool duplicated = false;
    const glm::vec3 duplicateOffset(1.0f, 0.0f, 0.0f);

    std::unordered_map<int, int> duplicatedPlanetIndices;
    YAML::Node planetSequence = stageYaml["planets"];
    if (planetSequence && planetSequence.IsSequence()) {
        for (const StageActorRef& target : targets) {
            if (target.type != StageActorType::Planet ||
                !selectedKeys.contains(StageActorQuery::MakeKey(target)) ||
                target.yamlIndex < 0 ||
                target.yamlIndex >= static_cast<int>(planetSequence.size())) {
                continue;
            }

            YAML::Node duplicatedPlanetNode =
                YAML::Clone(planetSequence[target.yamlIndex]);
            OffsetDuplicatedActorNode(
                duplicatedPlanetNode,
                duplicateOffset);

            const int duplicatedPlanetIndex =
                static_cast<int>(planetSequence.size());
            duplicatedPlanetNode["stageNum"] =
                duplicatedPlanetIndex;
            planetSequence.push_back(duplicatedPlanetNode);

            duplicatedPlanetIndices[target.yamlIndex] =
                duplicatedPlanetIndex;
            newSelectedKeys.insert(
                "planets:" +
                std::to_string(duplicatedPlanetIndex));
            duplicated = true;
        }
    }

    for (const StageActorRef& target : targets) {
        if (target.type == StageActorType::Planet) {
            continue;
        }

        const std::string key = StageActorQuery::MakeKey(target);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        YAML::Node sequence = stageYaml[target.sequenceName];

        if (!sequence || !sequence.IsSequence()) {
            std::cerr << "Duplicate skipped. Sequence not found: " << target.sequenceName << std::endl;
            continue;
        }

        if (target.yamlIndex < 0 || target.yamlIndex >= static_cast<int>(sequence.size())) {
            std::cerr << "Duplicate skipped. Invalid yamlIndex: " << target.yamlIndex << std::endl;
            continue;
        }

        YAML::Node sourceNode = sequence[target.yamlIndex];
        YAML::Node duplicatedNode = YAML::Clone(sourceNode);
        bool shouldOffsetActorPosition = true;

        if (target.type == StageActorType::Boat) {
            int startPlanetIndex = 0;
            if (TryReadPlanetIndex(
                    duplicatedNode,
                    "startPlanet",
                    0,
                    startPlanetIndex)) {
                const auto duplicatedStartPlanet =
                    duplicatedPlanetIndices.find(startPlanetIndex);
                if (duplicatedStartPlanet !=
                    duplicatedPlanetIndices.end()) {
                    duplicatedNode["startPlanet"] =
                        duplicatedStartPlanet->second;
                    shouldOffsetActorPosition = false;
                }
            }

            int destinationPlanetIndex = 0;
            if (TryReadPlanetIndex(
                    duplicatedNode,
                    "destPlanet",
                    0,
                    destinationPlanetIndex)) {
                const auto duplicatedDestinationPlanet =
                    duplicatedPlanetIndices.find(
                        destinationPlanetIndex);
                if (duplicatedDestinationPlanet !=
                    duplicatedPlanetIndices.end()) {
                    duplicatedNode["destPlanet"] =
                        duplicatedDestinationPlanet->second;
                }
            }
        } else {
            int currentPlanetIndex = 0;
            if (TryReadPlanetIndex(
                    duplicatedNode,
                    "currentPlanetNum",
                    0,
                    currentPlanetIndex)) {
                const auto duplicatedPlanet =
                    duplicatedPlanetIndices.find(
                        currentPlanetIndex);
                if (duplicatedPlanet !=
                    duplicatedPlanetIndices.end()) {
                    duplicatedNode["currentPlanetNum"] =
                        duplicatedPlanet->second;
                    shouldOffsetActorPosition = false;
                }
            }
        }

        if (shouldOffsetActorPosition) {
            OffsetDuplicatedActorNode(
                duplicatedNode,
                duplicateOffset);
        }
        if (target.type == StageActorType::Platform) {
            duplicatedNode["platformId"] =
                CreateUniquePlatformId(stageYaml);
        }

        const int newYamlIndex = static_cast<int>(sequence.size());

        sequence.push_back(duplicatedNode);

        newSelectedKeys.insert(target.sequenceName + ":" + std::to_string(newYamlIndex));

        duplicated = true;
    }

    if (!duplicated) {
        return false;
    }

    RenumberPlanetNodes(stageYaml["planets"]);

    PushUndo();

    if (!StageYamlRepository::SaveCurrentStage(mContext, stageYaml)) {
        return false;
    }

    mSelectionController.SetSelectedKeys(newSelectedKeys);

    mRequestOpenPlacement = true;

    mContext.game->ReloadCurrentStage();

    return true;
}

void StageEditCommandController::OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const
{
    if (!actorNode) {
        return;
    }

    const char* positionKey =
        actorNode["center"] ? "center" : "pos";
    if (actorNode[positionKey] &&
        actorNode[positionKey].IsSequence() &&
        actorNode[positionKey].size() >= 3) {
        try {
            const float x = actorNode[positionKey][0].as<float>();
            const float y = actorNode[positionKey][1].as<float>();
            const float z = actorNode[positionKey][2].as<float>();

            actorNode[positionKey][0] = x + offset.x;
            actorNode[positionKey][1] = y + offset.y;
            actorNode[positionKey][2] = z + offset.z;

            return;
        } catch (const YAML::Exception&) {
            std::cerr
                << "Invalid position while duplicating. Recreate position."
                << std::endl;
        }
    }

    if (actorNode["theta"]) {
        try {
            const float theta = actorNode["theta"].as<float>();
            actorNode["theta"] = theta + 0.15f;
        } catch (const YAML::Exception&) {
            actorNode["theta"] = 0.15f;
        }
    } else {
        actorNode["theta"] = 0.15f;
    }
}
