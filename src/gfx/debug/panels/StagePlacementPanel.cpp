#include "gfx/debug/panels/StagePlacementPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "imgui.h"

#include <utility>

StagePlacementPanel::StagePlacementPanel(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageActorYamlWriter& stageActorYamlWriter,
    Callback pushUndoCallback)
    : DebugPanel(context),
      mSelectionController(selectionController),
      mStagePlayerEditor(context),
      mStageActorInspector(
          context,
          selectionController,
          stageActorYamlWriter,
          mStagePlayerEditor,
          std::move(pushUndoCallback))
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
    ImGui::TextDisabled("一覧またはゲーム画面のモデルをクリックして選択します。");
    ImGui::TextDisabled("同じ場所を続けてクリックすると、手前から奥へ選択を切り替えます。");

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

    for (std::size_t i = 0; i < group.actors.size(); ++i) {
        const StageActorInstance& instance = group.actors[i];
        if (!instance.actor) {
            continue;
        }

        const bool selected = mSelectionController.IsSelected(instance.ref);
        const std::string displayLabel =
            instance.ref.label +
            (instance.actor->IsDebugDisabled()
                 ? " [デバッグ非表示]"
                 : "");
        const std::string selectableId =
            displayLabel + "##placementList_" +
            StageActorQuery::MakeKey(instance.ref);

        if (ImGui::Selectable(selectableId.c_str(), selected)) {
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
