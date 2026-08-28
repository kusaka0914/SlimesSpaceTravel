#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditorTypes.h"

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class Actor;
class Boat;
class NPC;

class StageActorAssetEditor {
public:
    using Callback = std::function<void()>;

    StageActorAssetEditor(
        DebugEditorContext& context,
        Callback rebuildPhysicsWorld);

    void DrawActorModelPicker(
        Actor* actor,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawNPCModelPicker(
        NPC* npc,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawBoatModelPicker(
        Boat* boat,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawTextureOverrideEditor(
        Actor* actor,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawBulkTextureOverrideEditor(
        const std::vector<StageActorInstance>& selectedActors);

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
