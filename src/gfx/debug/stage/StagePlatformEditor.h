#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StagePlatformTypeChanger.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class Platform;
class PlatformLatchedGroupSwitchComponent;
class StageActorYamlWriter;
class StageSelectionController;
struct PlatformTypeDefinition;

class StagePlatformEditor {
public:
    using Callback = std::function<void()>;

    StagePlatformEditor(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        StageActorYamlWriter& stageActorYamlWriter,
        Callback pushUndo,
        Callback rebuildPhysicsWorld);

    bool DrawPlatformTypeEditor(
        Platform* platform,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawPlatformBehaviorEditors(
        Platform* platform,
        int yamlIndex);

private:
    std::vector<std::string> CollectLatchedSwitchGroupIds() const;
    std::vector<Platform*> CollectLatchedSwitchGroupMembers(
        const std::string& groupId) const;
    PlatformLatchedGroupSwitchComponent*
    NormalizeLatchedSwitchGroupConfiguration(
        const std::string& groupId,
        bool& wasChanged) const;
    void RequestPhysicsWorldRebuild();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageActorYamlWriter& mStageActorYamlWriter;
    StagePlatformTypeChanger mPlatformTypeChanger;
    Callback mPushUndo;
    Callback mRebuildPhysicsWorld;
    std::string mPlatformTypeChangeStatus;
};
