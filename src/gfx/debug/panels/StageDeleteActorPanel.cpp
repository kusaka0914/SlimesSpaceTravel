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

    auto drawCategory = [&](const char* categoryName, StageActorType type) {
        if (!ImGui::TreeNode(categoryName)) {
            return;
        }

        bool hasItem = false;

        for (const StageActorRef& target : targets) {
            if (target.type != type) {
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

    drawCategory("敵", StageActorType::Enemy);
    drawCategory("足場", StageActorType::Platform);
    drawCategory("クリスタル", StageActorType::Crystal);
    drawCategory("NPC", StageActorType::NPC);
    drawCategory("ボートパーツ", StageActorType::BoatParts);
    drawCategory("ボート", StageActorType::Boat);
    drawCategory("キー", StageActorType::Key);
    drawCategory("星", StageActorType::Star);

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