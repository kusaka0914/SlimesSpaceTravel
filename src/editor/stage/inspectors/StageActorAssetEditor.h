#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageEditorTypes.h"

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class Actor;
class Boat;
class NPC;

// 選択Actorのモデル・テクスチャを編集する。Actorは所有せず、モデル変更時は必要に応じて物理を再構築する。
class StageActorAssetEditor {
public:
    using Callback = std::function<void()>;

    StageActorAssetEditor(DebugEditorContext& context, Callback rebuildPhysicsWorld);

    void DrawActorModelPicker(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    void DrawNPCModelPicker(NPC* npc, const std::string& sequenceName, std::size_t listIndex);
    void DrawBoatModelPicker(Boat* boat, const std::string& sequenceName, std::size_t listIndex);

    // テクスチャの上書き・解除を実行中Actorに反映する。YAMLへの保存は呼び出し元が行う。
    void DrawTextureOverrideEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);

    // 対応する選択Actorへまとめて反映する。選択内の未対応Actorは対象外とする。
    void DrawBulkTextureOverrideEditor(const std::vector<StageActorInstance>& selectedActors);

private:
    void RequestPhysicsWorldRebuild();

    DebugEditorContext& mContext;
    Callback mRebuildPhysicsWorld;
    std::array<char, 128> mTextureAssetFilter = {};
    std::array<char, 128> mActorModelAssetFilter = {};
    std::array<char, 128> mNPCModelAssetFilter = {};
    std::array<char, 128> mBoatModelAssetFilter = {};
    std::string mTextureAssetStatus;
};
