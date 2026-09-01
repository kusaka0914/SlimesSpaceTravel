#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <yaml-cpp/yaml.h>

struct DebugEditorContext;

class StageActorRuntimeCreationService {
public:
    explicit StageActorRuntimeCreationService(DebugEditorContext& context);

    bool CreateActor(
        StageActorType actorType,
        const YAML::Node& actorNode,
        int stageYamlIndex) const;
    bool CreateActor(
        const StageActorRef& actorRef,
        const YAML::Node& actorNode,
        int stageYamlIndex) const;
    void RefreshPhysicsWorld() const;

private:
    DebugEditorContext& mContext;
};
