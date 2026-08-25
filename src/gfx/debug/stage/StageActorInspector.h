#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorAssetEditor.h"
#include "gfx/debug/stage/StagePlatformEditor.h"

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <string>

class Actor;
class StageActorYamlWriter;
class StagePlayerEditor;
class StageSelectionController;

class StageActorInspector {
public:
    using Callback = std::function<void()>;

    StageActorInspector(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        StageActorYamlWriter& stageActorYamlWriter,
        StagePlayerEditor& stagePlayerEditor,
        Callback pushUndo);

    void Draw();

private:
    void DrawActorPlacementEditor(
        Actor* actor,
        const std::string& sequenceName,
        std::size_t listIndex);
    bool DrawActorTypeSettings(
        Actor* actor,
        const std::string& sequenceName,
        std::size_t listIndex,
        int yamlIndex);
    void DrawCommonActorSettings(
        Actor* actor,
        const std::string& sequenceName,
        std::size_t listIndex,
        int yamlIndex);
    glm::vec3 CalculateActorUpVecFromEditorRotation(
        Actor* actor,
        const glm::vec3& rotationRadians) const;
    void ApplyActorEditorRotation(Actor* actor);
    void RebuildPhysicsWorld();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageActorYamlWriter& mStageActorYamlWriter;
    StagePlayerEditor& mStagePlayerEditor;
    StageActorAssetEditor mStageActorAssetEditor;
    StagePlatformEditor mStagePlatformEditor;
    Callback mPushUndo;
    std::string mRubyGenerationStatus;
    std::string mSurfaceAlignmentStatus;
};
