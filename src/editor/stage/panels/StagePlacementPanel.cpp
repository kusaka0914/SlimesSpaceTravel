#include "editor/stage/panels/StagePlacementPanel.h"

#include "actor/Actor.h"
#include "app/Game.h"
#include "editor/assets/EditorAssetCatalog.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "imgui.h"
#include "world/Stage.h"

#include <utility>

StagePlacementPanel::StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController,
                                         StageActorYamlWriter& stageActorYamlWriter, SavedEditCallback recordSavedEdit)
    : DebugPanel(context),
      mSelectionController(selectionController),
      mStagePlayerEditor(context, recordSavedEdit),
      mStageActorInspector(context, selectionController, stageActorYamlWriter, mStagePlayerEditor,
                           std::move(recordSavedEdit))
{
}

void StagePlacementPanel::RequestOpenPickedActorPlacement()
{
    mRequestOpenPickedActorPlacement = true;
}

void StagePlacementPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    mStageActorInspector.Draw();
    mRequestOpenPickedActorPlacement = false;
}

void StagePlacementPanel::DrawObjectList()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::vector<ActorGroup> groups = CollectActorGroups();

    ImGui::SeparatorText("オブジェクト一覧");

    bool hasAnyActor = false;
    for (const ActorGroup& group : groups) {
        if (group.actors.empty()) {
            continue;
        }

        hasAnyActor = true;
        DrawActorList(group);
    }

    if (!hasAnyActor) {
        ImGui::TextDisabled("このステージには配置済みオブジェクトがありません。");
    }
}

void StagePlacementPanel::DrawPlayerSpawn()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    mStagePlayerEditor.DrawSpawnEditor();
}

void StagePlacementPanel::DrawPlayerDebugMover(Actor* selectedActor)
{
    mStagePlayerEditor.DrawDebugMover(selectedActor);
}

std::vector<StagePlacementPanel::ActorGroup> StagePlacementPanel::CollectActorGroups() const
{
    std::vector<ActorGroup> groups;

    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return groups;
    }

    for (const StageActorTypeInfo& info : StageActorQuery::GetTypeInfos()) {
        ActorGroup group;
        group.label = info.displayName;
        group.sequenceName = info.sequenceName;
        groups.emplace_back(group);
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        for (ActorGroup& group : groups) {
            if (group.sequenceName != instance.ref.sequenceName) {
                continue;
            }

            group.actors.emplace_back(instance);
            break;
        }
    }

    return groups;
}

void StagePlacementPanel::DrawActorList(const ActorGroup& group)
{
    if (group.actors.empty()) {
        return;
    }

    const std::string treeLabel = group.label + "##" + group.sequenceName;

    const auto& pickedActorRef = mSelectionController.GetPickedActorRef();

    if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == group.sequenceName) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode(treeLabel.c_str())) {
        return;
    }

    for (std::size_t index = 0; index < group.actors.size(); ++index) {
        const StageActorInstance& instance = group.actors[index];
        if (!instance.actor) {
            continue;
        }

        const bool isSelected = mSelectionController.IsSelected(instance.ref);
        const std::string displayLabel =
            instance.ref.label + (instance.actor->IsDebugDisabled() ? " [デバッグ非表示]" : "");
        const std::string selectableId = displayLabel + "##placementList_" + StageActorQuery::MakeKey(instance.ref);

        if (ImGui::Selectable(selectableId.c_str(), isSelected)) {
            const ImGuiIO& io = ImGui::GetIO();
            if (io.KeyCtrl || io.KeyShift) {
                mSelectionController.ToggleSelection(instance.actor, instance.ref);
            } else {
                mSelectionController.SetSingleSelection(instance.actor, instance.ref);
            }
        }
    }

    ImGui::TreePop();
}
