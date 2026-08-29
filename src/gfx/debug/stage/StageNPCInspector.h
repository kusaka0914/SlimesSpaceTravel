#pragma once

#include <cstddef>
#include <string>

class NPC;
class StageActorAssetEditor;
struct DebugEditorContext;

class StageNPCInspector {
public:
    StageNPCInspector(
        DebugEditorContext& context,
        StageActorAssetEditor& assetEditor);

    void Draw(
        NPC* npc,
        const std::string& sequenceName,
        std::size_t listIndex,
        int yamlIndex);

private:
    DebugEditorContext& mContext;
    StageActorAssetEditor& mAssetEditor;
    std::string mRubyGenerationStatus;
};
