#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>
#include <yaml-cpp/yaml.h>

class Actor;
class Planet;
struct EditorAuthoredTransform;

class StageActorYamlWriter {
public:
    explicit StageActorYamlWriter(DebugEditorContext& context);

    void SaveAllActorStates();
    void SaveEditorAuthoredTransforms();
    void WriteAllActorStates(YAML::Node& config);

private:
    enum class ActorTransformWriteMode {
        RuntimeState,
        EditorAuthoredTransform,
    };

    void WriteActorState(
        YAML::Node& config,
        const std::string& sequenceName,
        Actor* actor,
        ActorTransformWriteMode transformWriteMode);
    void WriteModelAndHazardState(YAML::Node actorNode, Actor* actor);
    void WritePlatformState(
        YAML::Node actorNode,
        const std::string& sequenceName,
        Actor* actor,
        const EditorAuthoredTransform* editorTransform);
    void WriteBoatState(
        YAML::Node actorNode,
        Actor* actor,
        const EditorAuthoredTransform* editorTransform,
        Planet* authoredPlanet);
    void WriteNPCState(YAML::Node actorNode, Actor* actor);
    void WriteTextureState(YAML::Node actorNode, Actor* actor);
    int FindPlanetIndex(const Planet* targetPlanet) const;

    DebugEditorContext& mContext;
};
