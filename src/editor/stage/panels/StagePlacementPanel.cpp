#include "editor/stage/panels/StagePlacementPanel.h"

#include "actor/Actor.h"
#include "app/Game.h"
#include "editor/assets/EditorAssetCatalog.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "imgui.h"
#include "world/Stage.h"

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <utility>

namespace {
bool HasSearchQuery(std::string_view query)
{
    return query.find_first_not_of(" \t\n\r\f\v") != std::string_view::npos;
}

std::string PlanetNodeKey(const StageHierarchyPlanetGroup& planet)
{
    return "planet:" + std::to_string(planet.planetIndex);
}

std::string GroupNodeKey(const StageHierarchyPlanetGroup& planet, const StageHierarchyActorGroup& group)
{
    return PlanetNodeKey(planet) + "/category:" + std::to_string(static_cast<int>(group.category));
}

bool ContainsActor(const std::vector<StageHierarchyPlanetGroup>& hierarchy, const std::string& key)
{
    for (const auto& planet : hierarchy) {
        if (planet.planet.actor && StageActorQuery::MakeKey(planet.planet.ref) == key) {
            return true;
        }
        for (const auto& group : planet.groups) {
            for (const auto& actor : group.actors) {
                if (StageActorQuery::MakeKey(actor.ref) == key) {
                    return true;
                }
            }
        }
    }
    return false;
}
} // namespace

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
}

void StagePlacementPanel::DrawObjectList()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const auto hierarchy = StageActorHierarchy::Collect(mContext.game->GetCurrentStage());
    const auto stageKey =
        std::to_string(mContext.game->GetCurrentStageNum()) + ":" +
        std::filesystem::path(mContext.game->GetCurrentStageYamlPath()).lexically_normal().generic_string();
    auto& state = mHierarchyViews[stageKey];
    ImGui::PushID(stageKey.c_str());
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##hierarchySearch", "名前・種類・惑星番号で検索", state.search.data(),
                             state.search.size());

    if (std::exchange(mRequestOpenPickedActorPlacement, false)) {
        RevealSelection(hierarchy, state);
    }
    const auto visible = StageActorHierarchy::Filter(hierarchy, state.search.data());
    const bool isSearching = HasSearchQuery(state.search.data());
    ImGui::BeginChild("HierarchyTree", ImVec2(0, 0));
    for (const auto& planet : visible) {
        DrawPlanetGroup(planet, state, isSearching);
    }
    if (visible.empty()) {
        ImGui::TextDisabled(isSearching ? "一致するオブジェクトがありません。" : "このステージには惑星がありません。");
    }
    ImGui::EndChild();
    ImGui::PopID();
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

void StagePlacementPanel::RevealSelection(const std::vector<StageHierarchyPlanetGroup>& hierarchy,
                                          HierarchyViewState& state)
{
    mRevealActorKey.clear();
    const auto& picked = mSelectionController.GetPickedActorRef();
    if (picked && mSelectionController.IsSelected(*picked)) {
        mRevealActorKey = StageActorQuery::MakeKey(*picked);
    } else {
        for (const auto& key : mSelectionController.GetSelectedKeys()) {
            if (ContainsActor(hierarchy, key)) {
                mRevealActorKey = key;
                break;
            }
        }
    }
    if (mRevealActorKey.empty() || !ContainsActor(hierarchy, mRevealActorKey)) {
        mRevealActorKey.clear();
        return;
    }
    // Game Viewで選んだ対象が検索に隠れている場合は、対象を確認できることを優先する。
    if (!ContainsActor(StageActorHierarchy::Filter(hierarchy, state.search.data()), mRevealActorKey)) {
        state.search.fill('\0');
    }
    if (HasSearchQuery(state.search.data())) {
        return;
    }
    for (const auto& planet : hierarchy) {
        if (planet.planet.actor && mSelectionController.IsSelected(planet.planet.ref)) {
            state.openNodes[PlanetNodeKey(planet)] = true;
        }
        for (const auto& group : planet.groups) {
            if (std::any_of(group.actors.begin(), group.actors.end(),
                            [this](const auto& instance) { return mSelectionController.IsSelected(instance.ref); })) {
                state.openNodes[PlanetNodeKey(planet)] = true;
                state.openNodes[GroupNodeKey(planet, group)] = true;
            }
        }
    }
}

bool StagePlacementPanel::BeginHierarchyNode(const std::string& key, const std::string& label, ImGuiTreeNodeFlags flags,
                                             HierarchyViewState& state, bool defaultOpen, bool isSearching)
{
    const auto node = state.openNodes.try_emplace(key, defaultOpen).first;
    ImGui::SetNextItemOpen(isSearching || node->second, ImGuiCond_Always);
    const bool isOpen = ImGui::TreeNodeEx(key.c_str(), flags, "%s", label.c_str());
    // 検索中の強制展開・クリックは通常表示の開閉状態へ書き戻さない。
    if (!isSearching && ImGui::IsItemToggledOpen()) {
        node->second = isOpen;
    }
    return isOpen;
}

void StagePlacementPanel::DrawPlanetGroup(const StageHierarchyPlanetGroup& planet, HierarchyViewState& state,
                                          bool isSearching)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
    if (planet.planet.actor) {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        if (mSelectionController.IsSelected(planet.planet.ref)) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
    }
    const auto label = planet.label + " (" + std::to_string(planet.actorCount) + ")" +
                       (planet.planet.actor && planet.planet.actor->IsDebugDisabled() ? " [デバッグ非表示]" : "");
    const bool isOpen = BeginHierarchyNode(PlanetNodeKey(planet), label, flags, state, true, isSearching);
    if (planet.planet.actor) {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
            SelectActor(planet.planet);
        }
        ScrollToRevealedActor(planet.planet);
    }
    if (!isOpen) {
        return;
    }
    for (const auto& group : planet.groups) {
        const auto groupLabel = group.label + " (" + std::to_string(group.actorCount) + ")";
        if (BeginHierarchyNode(GroupNodeKey(planet, group), groupLabel, ImGuiTreeNodeFlags_SpanAvailWidth, state, false,
                               isSearching)) {
            for (const auto& actor : group.actors) {
                DrawActorRow(actor);
            }
            ImGui::TreePop();
        }
    }
    ImGui::TreePop();
}

void StagePlacementPanel::DrawActorRow(const StageActorInstance& instance)
{
    const auto label = instance.ref.label + (instance.actor->IsDebugDisabled() ? " [デバッグ非表示]" : "") +
                       "###actor_" + StageActorQuery::MakeKey(instance.ref);
    if (ImGui::Selectable(label.c_str(), mSelectionController.IsSelected(instance.ref))) {
        SelectActor(instance);
    }
    ScrollToRevealedActor(instance);
}

void StagePlacementPanel::SelectActor(const StageActorInstance& instance)
{
    const auto& io = ImGui::GetIO();
    if (io.KeyCtrl || io.KeyShift) {
        mSelectionController.ToggleSelection(instance.actor, instance.ref);
    } else {
        mSelectionController.SetSingleSelection(instance.actor, instance.ref);
    }
}

void StagePlacementPanel::ScrollToRevealedActor(const StageActorInstance& instance)
{
    if (StageActorQuery::MakeKey(instance.ref) == mRevealActorKey) {
        ImGui::SetScrollHereY(0.5f);
        mRevealActorKey.clear();
    }
}
