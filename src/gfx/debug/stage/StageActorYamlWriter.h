#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>
#include <yaml-cpp/yaml.h>

class Actor;
class Planet;
struct EditorAuthoredTransform;

// 実行中Actorの設定と、エディターで確定した配置を既存のステージYAMLへ反映する。惑星は別途保存する。
class StageActorYamlWriter {
public:
    explicit StageActorYamlWriter(DebugEditorContext& context);

    // 惑星以外の実行中Actorの状態を保存する。読み込みに失敗した場合は保存しない。
    void SaveAllActorStates();

    // 編集記録のあるActorとPlayerだけを保存し、保存成功後にその編集記録を消去する。
    void SaveEditorAuthoredTransforms();

    // 既存configへ実行中Actorの状態を反映する。ファイルには書き込まない。
    void WriteAllActorStates(YAML::Node& config);

private:
    enum class ActorTransformWriteMode {
        RuntimeState,
        EditorAuthoredTransform,
    };

    void WriteActorState(YAML::Node& config, const std::string& sequenceName, Actor* actor,
                         ActorTransformWriteMode transformWriteMode);
    void WriteModelAndHazardState(YAML::Node actorNode, Actor* actor);
    void WritePlatformState(YAML::Node actorNode, const std::string& sequenceName, Actor* actor,
                            const EditorAuthoredTransform* editorTransform);
    void WriteBoatState(YAML::Node actorNode, Actor* actor, const EditorAuthoredTransform* editorTransform,
                        Planet* authoredPlanet);
    void WriteNPCState(YAML::Node actorNode, Actor* actor);
    void WriteTextureState(YAML::Node actorNode, Actor* actor);
    int FindPlanetIndex(const Planet* targetPlanet) const;

    DebugEditorContext& mContext;
};
