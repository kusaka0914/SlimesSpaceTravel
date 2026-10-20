#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/stage/inspectors/StageActorInspector.h"
#include "editor/stage/inspectors/StagePlayerEditor.h"
#include "editor/stage/selection/StageSelectionController.h"

#include <functional>
#include <string>
#include <vector>

class Actor;
class StageActorYamlWriter;

// 配置済みActorの一覧・選択・インスペクターを連携する。selectionControllerとYAML Writerは所有しない。
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

    // 次の一覧描画で選択Actorのカテゴリを開く。Drawで要求を解除するため、一覧を先に描画する。
    void RequestOpenPickedActorPlacement();

private:
    struct ActorGroup {
        std::string label;
        std::string sequenceName;
        std::vector<StageActorInstance> actors;
    };

    std::vector<ActorGroup> CollectActorGroups() const;

    void DrawActorList(const ActorGroup& group);

    StageSelectionController& mSelectionController;
    StagePlayerEditor mStagePlayerEditor;
    StageActorInspector mStageActorInspector;
    bool mRequestOpenPickedActorPlacement = false;
};
