#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include "gfx/debug/stage/StageActorSaveState.h"

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Actor;
class Planet;
struct EditorAuthoredTransform;

// Actor設定と編集済み配置をステージYAMLへ反映する。惑星のシリアライズは専用Writerへ委譲する。
class StageActorYamlWriter {
public:
    using SavedEditCallback = StageActorSaveState::SavedEditCallback;

    explicit StageActorYamlWriter(DebugEditorContext& context);

    // 惑星以外の実行中Actorを保存する。成功後だけ変更前YAMLを通知する。失敗時はfalseで、Runtimeは維持する。
    bool SaveAllActorStates(const SavedEditCallback& recordSavedEdit = {});

    // Actor・惑星設定と未保存Transformを1回のファイル置換で保存する。失敗時は全編集記録を保持する。
    bool SaveStage(const SavedEditCallback& recordSavedEdit = {});

    // 編集記録のあるActor・Player・惑星のTransformを保存し、成功後に記録を消去する。変更がなければtrue。
    bool SaveEditorAuthoredTransforms(const SavedEditCallback& recordSavedEdit = {});

    // 失敗した保存を同じステージで再試行する。成功通知は再試行まで保持し、Actorの再読込は行わない。
    // 再試行・破棄まで通知先の寿命を保証し、コールバックからRuntimeを破棄しないこと。
    bool RetryFailedSaves();
    const std::string& GetSaveError() const;

    // Runtime再読込成功後に呼ぶ。失敗時の再試行記録は維持する。
    void ClearFailedSaves();

    // 既存configへ実行中Actorの状態を反映する。ファイルには書き込まない。
    void WriteAllActorStates(YAML::Node& config);

    // 編集済みTransformをconfigへ反映する。保存・編集記録の解除はせず、対象不正時はfalse。
    bool WriteEditorAuthoredTransforms(YAML::Node& config);

private:
    using SaveScope = StageActorSaveState::Scope;

    bool WriteEditorAuthoredTransforms(YAML::Node& config, std::vector<Actor*>& writtenActors);
    bool SaveActorStates(SaveScope scope, const SavedEditCallback& recordSavedEdit);

    enum class ActorTransformWriteMode {
        RuntimeState,
        EditorAuthoredTransform,
    };

    bool WriteActorState(YAML::Node& config, const std::string& sequenceName, Actor* actor,
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
    StageActorSaveState mSaveState;
};
