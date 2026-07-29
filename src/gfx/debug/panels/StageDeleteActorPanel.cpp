#include "gfx/debug/panels/StageDeleteActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "imgui.h"

#include <string>
#include <vector>

StageDeleteActorPanel::StageDeleteActorPanel(DebugEditorContext& context,
                                             StageEditCommandController& editCommandController)
    : DebugPanel(context),
      mEditCommandController(editCommandController)
{
}

void StageDeleteActorPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    std::vector<StageActorRef> targets = StageActorQuery::CollectAllTargets(mContext.game->GetCurrentStage());

    if (targets.empty()) {
        ImGui::Text("削除できるオブジェクトがありません");
        return;
    }

    auto drawCategory = [&](const StageActorTypeInfo& typeInfo) {
        if (!ImGui::TreeNode(typeInfo.displayName.c_str())) {
            return;
        }

        bool hasItem = false;

        for (const StageActorRef& target : targets) {
            if (target.sequenceName != typeInfo.sequenceName) {
                continue;
            }

            hasItem = true;

            const std::string key = StageActorQuery::MakeKey(target);
            bool selected = mSelectedKeys.contains(key);

            const std::string checkboxLabel = target.label + "##delete_" + key;

            if (ImGui::Checkbox(checkboxLabel.c_str(), &selected)) {
                if (selected) {
                    mSelectedKeys.insert(key);
                } else {
                    mSelectedKeys.erase(key);
                }
            }
        }

        if (!hasItem) {
            ImGui::Text("なし");
        }

        ImGui::TreePop();
    };

    for (const StageActorTypeInfo& info : StageActorQuery::GetTypeInfos()) {
        drawCategory(info);
    }

    ImGui::Separator();

    ImGui::Text("選択数: %d", static_cast<int>(mSelectedKeys.size()));

    const bool canDelete = !mSelectedKeys.empty();

    if (!canDelete) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("選択中のオブジェクトを削除")) {
        ImGui::OpenPopup("削除確認");
    }

    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (ImGui::BeginPopupModal("削除確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("選択中のオブジェクトを削除します。よろしいですか？");
        ImGui::Text("削除数: %d", static_cast<int>(mSelectedKeys.size()));

        if (ImGui::Button("削除する")) {
            mEditCommandController.DeleteSelectedKeys(mSelectedKeys);
            mSelectedKeys.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("キャンセル")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
