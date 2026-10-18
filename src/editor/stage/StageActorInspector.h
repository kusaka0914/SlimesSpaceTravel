#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageActorAssetEditor.h"
#include "editor/stage/StageNPCInspector.h"
#include "editor/stage/StagePlatformEditor.h"

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <string>

class Actor;
class StageActorYamlWriter;
class StagePlayerEditor;
class StageSelectionController;

// 選択Actorの設定UIを統括し、種類別の編集を各エディターへ委譲する。参照先のActorやサービスは所有しない。
class StageActorInspector {
public:
    using SavedEditCallback = std::function<void(const std::string&)>;

    StageActorInspector(DebugEditorContext& context, StageSelectionController& selectionController,
                        StageActorYamlWriter& stageActorYamlWriter, StagePlayerEditor& stagePlayerEditor,
                        SavedEditCallback recordSavedEdit);

    // Gameと現在のStageが有効な状態で呼び、現在の選択に応じて編集UIを描画する。
    void Draw();

private:
    void DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);

    // 再読込が起こり得る種類変更を試みたらtrue。以降は元のActorを参照しない。
    bool DrawActorTypeSettings(Actor* actor, const std::string& sequenceName, std::size_t listIndex, int yamlIndex);
    void DrawCommonActorSettings(Actor* actor, const std::string& sequenceName, std::size_t listIndex, int yamlIndex);
    void DrawMovingPlatformSettings(Actor* actor, int yamlIndex);
    void DrawBoatSettings(Actor* actor, const std::string& sequenceName, std::size_t listIndex, int yamlIndex);
    void DrawVisibilityAndGravitySettings(Actor* actor, const std::string& sequenceName, int yamlIndex);
    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRadians) const;
    void ApplyActorEditorRotation(Actor* actor);
    void RebuildPhysicsWorld();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageActorYamlWriter& mStageActorYamlWriter;
    StagePlayerEditor& mStagePlayerEditor;
    StageActorAssetEditor mStageActorAssetEditor;
    StageNPCInspector mStageNPCInspector;
    StagePlatformEditor mStagePlatformEditor;
    SavedEditCallback mRecordSavedEdit;
    std::string mSurfaceAlignmentStatus;
};
