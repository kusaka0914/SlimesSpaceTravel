#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/stage/StageActorHierarchy.h"
#include "editor/stage/inspectors/StageActorInspector.h"
#include "editor/stage/inspectors/StagePlayerEditor.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "imgui.h"

#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Actor;
class StageActorYamlWriter;

// 惑星別のHierarchy・検索・インスペクターを連携する。選択管理は借用し、検索・開閉状態だけを保持する。
class StagePlacementPanel : public DebugPanel {
public:
    using SavedEditCallback = std::function<void(const std::string&)>;

    StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController,
                        StageActorYamlWriter& stageActorYamlWriter, SavedEditCallback recordSavedEdit = {});

    // 選択対象の設定を編集する。Game・ステージ・アセットカタログが未設定なら編集しない。
    void Draw() override;
    void DrawObjectList();
    void DrawPlayerSpawn();
    void DrawPlayerDebugMover(Actor* selectedActor);

    // 次の一覧描画で選択Actorの惑星・種類を開き、対象行へスクロールする。
    void RequestOpenPickedActorPlacement();

private:
    struct HierarchyViewState {
        std::array<char, 256> search{};
        std::unordered_map<std::string, bool> openNodes;
    };

    void RevealSelection(const std::vector<StageHierarchyPlanetGroup>& hierarchy, HierarchyViewState& state);
    void DrawPlanetGroup(const StageHierarchyPlanetGroup& planet, HierarchyViewState& state, bool isSearching);
    void DrawActorRow(const StageActorInstance& instance);
    void SelectActor(const StageActorInstance& instance);
    void ScrollToRevealedActor(const StageActorInstance& instance);
    bool BeginHierarchyNode(const std::string& key, const std::string& label, ImGuiTreeNodeFlags flags,
                            HierarchyViewState& state, bool defaultOpen, bool isSearching);

    StageSelectionController& mSelectionController;
    StagePlayerEditor mStagePlayerEditor;
    StageActorInspector mStageActorInspector;
    std::unordered_map<std::string, HierarchyViewState> mHierarchyViews;
    std::string mRevealActorKey;
    bool mRequestOpenPickedActorPlacement = false;
};
