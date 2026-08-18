#include "gfx/debug/stage/StageEditCommandController.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include "imgui.h"

#include <GLFW/glfw3.h>
#include <algorithm>
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

bool IsPrimaryShortcutModifierPressed(GLFWwindow* window)
{
    if (!window) {
        return false;
    }

#if defined(__APPLE__)
    return glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
#else
    return glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
#endif
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
            playerNode["currentPlanetNum"] =
                replacementPlanetIndex;
        } else if (currentPlanetIndex > deletedPlanetIndex) {
            playerNode["currentPlanetNum"] =
                currentPlanetIndex - 1;
        }
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

void StageEditCommandController::UpdateShortcuts()
{
    HandleDeleteShortcut();
    HandleUndoShortcut();
    HandleDuplicateShortcut();
}

bool StageEditCommandController::ConsumeRequestOpenPlacement()
{
    const bool result = mRequestOpenPlacement;
    mRequestOpenPlacement = false;
    return result;
}

void StageEditCommandController::HandleDeleteShortcut()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool backspacePressed = ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
    const bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete, false);

    if (!backspacePressed && !deletePressed) {
        return;
    }

    DeleteSelectedKeys(mSelectionController.GetSelectedKeys());
}

void StageEditCommandController::HandleUndoShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed =
        IsPrimaryShortcutModifierPressed(mContext.game->GetWindow());

    const bool zPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_Z) == GLFW_PRESS;

    const bool zTriggered = zPressed && !mZPressedPrev;
    mZPressedPrev = zPressed;

    if (!commandPressed || !zTriggered) {
        return;
    }

    RestoreUndo();
}

void StageEditCommandController::HandleDuplicateShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed =
        IsPrimaryShortcutModifierPressed(mContext.game->GetWindow());

    const bool dPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_D) == GLFW_PRESS;

    const bool dTriggered = dPressed && !mDPressedPrev;
    mDPressedPrev = dPressed;

    if (!commandPressed || !dTriggered) {
        return;
    }

    DuplicateSelectedKeys(mSelectionController.GetSelectedKeys());
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

    mUndoStack.push_back(yamlText);

    constexpr std::size_t maxUndoCount = 20;
    if (mUndoStack.size() > maxUndoCount) {
        mUndoStack.erase(mUndoStack.begin());
    }
}

bool StageEditCommandController::RestoreUndo()
{
    if (!mContext.game || mUndoStack.empty()) {
        return false;
    }

    const std::string yamlText = mUndoStack.back();

    if (!StageYamlRepository::WriteCurrentStageTextAtomically(mContext, yamlText)) {
        return false;
    }

    mUndoStack.pop_back();

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

    // Switch target references currently use a sequence name plus YAML index.
    // Keep references to surviving actors valid before sequence entries shift.
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
        if (typeInfo.sequenceName != "boats") {
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

    if (!StageYamlRepository::SaveCurrentStage(
            mContext,
            stageYaml)) {
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
