#pragma once

#include "editor/stage/StageEditorTypes.h"

#include <yaml-cpp/yaml.h>

struct DebugEditorContext;

// YAMLノードからActorLoadSystem経由でActorを生成する。ファイル保存は担当しない。
class StageActorRuntimeCreationService {
public:
    explicit StageActorRuntimeCreationService(DebugEditorContext& context);

    // 生成したActorの所有権は既存の生成先に従う。未対応の種類や生成失敗時は false。
    bool CreateActor(StageActorType actorType, const YAML::Node& actorNode, int stageYamlIndex) const;

    // sequenceName に応じて通常足場と旧形式の movingPlatforms を区別して生成する。
    bool CreateActor(const StageActorRef& actorRef, const YAML::Node& actorNode, int stageYamlIndex) const;

    // 有効なPhysicsSystemがある場合に物理ワールドを再初期化する。
    void RefreshPhysicsWorld() const;

private:
    DebugEditorContext& mContext;
};
