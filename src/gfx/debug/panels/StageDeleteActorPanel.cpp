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
    } else {
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

                const std::string key =
                    StageActorQuery::MakeKey(target);
                bool selected = mSelectedKeys.contains(key);

                const std::string checkboxLabel =
                    target.label + "##delete_" + key;

                if (ImGui::Checkbox(
                        checkboxLabel.c_str(),
                        &selected)) {
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

        for (const StageActorTypeInfo& info :
             StageActorQuery::GetTypeInfos()) {
            drawCategory(info);
        }

        ImGui::Separator();

        ImGui::Text(
            "選択数: %d",
            static_cast<int>(mSelectedKeys.size()));

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

        if (ImGui::BeginPopupModal(
                "削除確認",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(
                "選択中のオブジェクトを削除します。よろしいですか？");
            ImGui::Text(
                "削除数: %d",
                static_cast<int>(mSelectedKeys.size()));

            if (ImGui::Button("削除する")) {
                mEditCommandController.DeleteSelectedKeys(
                    mSelectedKeys);
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

    DrawPlanetDeletion();
}

void StageDeleteActorPanel::DrawPlanetDeletion()
{
    Stage* stage =
        mContext.game ? mContext.game->GetCurrentStage() : nullptr;
    if (!stage) {
        return;
    }

    const std::vector<Planet*>& planets = stage->GetPlanets();

    ImGui::SeparatorText("惑星の削除");
    if (planets.size() <= 1) {
        mSelectedPlanetIndex = -1;
        ImGui::TextDisabled(
            "ステージに必要な最後の惑星は削除できません。");
        return;
    }

    if (mSelectedPlanetIndex < 0 ||
        mSelectedPlanetIndex >= static_cast<int>(planets.size())) {
        mSelectedPlanetIndex = 0;
    }

    const std::string planetPreview =
        "惑星 " + std::to_string(mSelectedPlanetIndex);
    if (ImGui::BeginCombo(
            "削除する惑星",
            planetPreview.c_str())) {
        for (int planetIndex = 0;
             planetIndex < static_cast<int>(planets.size());
             ++planetIndex) {
            const bool isSelected =
                planetIndex == mSelectedPlanetIndex;
            const std::string label =
                "惑星 " + std::to_string(planetIndex);
            if (ImGui::Selectable(
                    label.c_str(),
                    isSelected)) {
                mSelectedPlanetIndex = planetIndex;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::TextWrapped(
        "削除した惑星上の配置物と、その惑星を経路に含む"
        "ロケットも削除されます。後ろの惑星番号は自動で詰めます。");

    if (ImGui::Button("選択した惑星を削除")) {
        mPendingPlanetIndex = mSelectedPlanetIndex;
        ImGui::OpenPopup("惑星削除確認");
    }

    if (!ImGui::BeginPopupModal(
            "惑星削除確認",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::Text(
        "惑星 %d と関連する配置物を削除します。",
        mPendingPlanetIndex);
    ImGui::TextUnformatted(
        "この操作は Ctrl+Z で元に戻せます。");

    if (ImGui::Button("惑星を削除する")) {
        if (mEditCommandController.DeletePlanet(
                mPendingPlanetIndex)) {
            mSelectedPlanetIndex = -1;
        }
        mPendingPlanetIndex = -1;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("キャンセル##planetDelete")) {
        mPendingPlanetIndex = -1;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
