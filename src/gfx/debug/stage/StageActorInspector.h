#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorAssetEditor.h"
#include "gfx/debug/stage/StageNPCInspector.h"
#include "gfx/debug/stage/StagePlatformEditor.h"

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
    using Callback = std::function<void()>;

    StageActorInspector(DebugEditorContext& context, StageSelectionController& selectionController,
                        StageActorYamlWriter& stageActorYamlWriter, StagePlayerEditor& stagePlayerEditor,
                        Callback pushUndo);

    // Gameと現在のStageが有効な状態で呼び、現在の選択に応じて編集UIを描画する。
    void Draw();

private:
    void DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);

    // 種類変更でステージを再読み込みした場合は true。以降は元のActorを参照しない。
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
    Callback mPushUndo;
    std::string mSurfaceAlignmentStatus;
};
