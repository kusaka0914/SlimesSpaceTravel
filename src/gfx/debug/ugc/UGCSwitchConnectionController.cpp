#include "gfx/debug/ugc/UGCSwitchConnectionController.h"

#include "Game.h"
#include "actor/Platform.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"

#include <string>
#include <yaml-cpp/yaml.h>

UGCSwitchConnectionController::UGCSwitchConnectionController(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageSelectionController& selectionController,
    UGCEditorToolState& toolState,
    UGCSwitchConnectionState& connectionState)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mSelectionController(selectionController),
      mToolState(toolState),
      mConnectionState(connectionState)
{
}

bool UGCSwitchConnectionController::CompletePendingConnection(
    Actor* selectedActor,
    const std::optional<StageActorRef>& selectedRef)
{
    const std::optional<StageActorRef>& switchRef =
        mConnectionState.GetSwitchRef();
    if (!switchRef || !selectedRef) {
        return false;
    }

    Platform* targetPlatform = dynamic_cast<Platform*>(selectedActor);
    const bool selectedIsConnectionSwitch =
        selectedRef->sequenceName == switchRef->sequenceName &&
        selectedRef->yamlIndex == switchRef->yamlIndex;
    const bool selectedIsAnotherSwitch = targetPlatform &&
        (targetPlatform->GetPressureSwitchComponent() != nullptr ||
         targetPlatform->GetLatchedGroupSwitchComponent() != nullptr);
    const bool canEditSelectedPlatformConnection =
        !selectedIsConnectionSwitch &&
        selectedRef->sequenceName == "platforms" &&
        targetPlatform &&
        !selectedIsAnotherSwitch;
    if (!canEditSelectedPlatformConnection) {
        mToolState.statusMessage =
            "足場を選択してください（惑星やスイッチにはつなげません）";
        mSelectionController.Clear();
        return true;
    }

    YAML::Node stageYaml;
    const bool loaded =
        StageYamlRepository::LoadCurrentStage(mContext, stageYaml);
    const YAML::Node switchNodes = loaded
        ? stageYaml[switchRef->sequenceName]
        : YAML::Node();
    const YAML::Node targetNodes = loaded
        ? stageYaml[selectedRef->sequenceName]
        : YAML::Node();
    const bool hasValidNodes =
        switchNodes && targetNodes &&
        switchNodes.IsSequence() && targetNodes.IsSequence() &&
        switchRef->yamlIndex >= 0 &&
        selectedRef->yamlIndex >= 0 &&
        switchRef->yamlIndex < static_cast<int>(switchNodes.size()) &&
        selectedRef->yamlIndex < static_cast<int>(targetNodes.size());
    if (hasValidNodes) {
        const YAML::Node targetNode = targetNodes[selectedRef->yamlIndex];
        const std::string targetId = targetNode["platformId"]
            ? targetNode["platformId"].as<std::string>()
            : "";
        const bool wasConnectionChanged =
            mConnectionState.GetAction() ==
                UGCSwitchConnectionAction::Connect
            ? StagePlatformConnections::AssignExclusiveSwitchTarget(
                  stageYaml,
                  switchRef->yamlIndex,
                  targetId)
            : StagePlatformConnections::DisconnectSwitchTarget(
                  stageYaml,
                  switchRef->yamlIndex,
                  targetId,
                  selectedRef->yamlIndex);
        if (wasConnectionChanged) {
            StageYamlRepository::SaveCurrentStage(mContext, stageYaml);
            if (mContext.game) {
                mContext.game->ReloadCurrentStage(
                    StagePhysicsReloadMode::SkipRebuild);
            }
            mToolState.statusMessage =
                mConnectionState.GetAction() ==
                    UGCSwitchConnectionAction::Connect
                ? "スイッチと足場をつなぎました"
                : "スイッチと足場のつながりを解除しました";
        } else if (
            mConnectionState.GetAction() ==
            UGCSwitchConnectionAction::Disconnect) {
            mToolState.statusMessage =
                "この足場とはつながっていません";
        }
    }

    mConnectionState.Cancel();
    mSelectionController.Clear();
    return true;
}

void UGCSwitchConnectionController::BeginConnection(
    const StageActorRef& switchRef,
    UGCSwitchConnectionAction action)
{
    mToolState.DeactivateEraser();
    mStageAddActorPanel.CancelPlacement();
    mConnectionState.Begin(switchRef, action);
    mSelectionController.Clear();
    mToolState.statusMessage =
        action == UGCSwitchConnectionAction::Connect
        ? "表示したい足場を1つクリックしてください"
        : "つながりを解除したい足場をクリックしてください";
}

