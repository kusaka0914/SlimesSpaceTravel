#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <cstddef>
#include <functional>
#include <string>

class StageActorYamlWriter;
class StageSelectionController;
struct PlatformTypeDefinition;

class StagePlatformTypeChanger {
public:
    using Callback = std::function<void()>;

    StagePlatformTypeChanger(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        StageActorYamlWriter& stageActorYamlWriter,
        Callback pushUndo);

    bool ChangePlatformType(
        const std::string& sourceSequenceName,
        std::size_t sourceIndex,
        const PlatformTypeDefinition& targetType);

private:
    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageActorYamlWriter& mStageActorYamlWriter;
    Callback mPushUndo;
};
