#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <optional>
#include <string>
#include <vector>

class Actor;
class Stage;

class StageActorQuery {
public:
    static const std::vector<StageActorTypeInfo>& GetTypeInfos();

    static std::vector<StageActorInstance> CollectAllActorInstances(Stage* stage);
    static std::vector<StageActorRef> CollectAllTargets(Stage* stage);

    static std::optional<StageActorRef> FindTargetForActor(Stage* stage, Actor* actor);
    static Actor* FindActorByRef(Stage* stage, const StageActorRef& target);

    static std::string MakeKey(const StageActorRef& target);

    static std::string GetSequenceName(StageActorType type);
    static const char* GetTypeLabel(StageActorType type);
};