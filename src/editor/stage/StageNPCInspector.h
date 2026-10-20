#pragma once

#include <cstddef>
#include <string>
#include <vector>

class NPC;
class StageActorAssetEditor;
struct DebugEditorContext;
struct StageActorInstance;

// NPCとTutorialTriggerの会話・ルビ・カメラフォーカスを編集する。渡されたNPCは所有しない。
class StageNPCInspector {
public:
    StageNPCInspector(DebugEditorContext& context, StageActorAssetEditor& assetEditor);

    // Game・現在のStage・npcのいずれかが未設定なら何もしない。
    // 会話の変更は実行中NPCへ反映し、YAMLへの保存は呼び出し元が行う。
    void Draw(NPC* npc, const std::string& sequenceName, std::size_t listIndex, int yamlIndex);

private:
    void DrawInlineConversationEditor(NPC* npc, bool isTutorialTrigger, int yamlIndex);

    // ページを削除した場合は true。呼び出し元は添字が変わるためページ走査を打ち切る。
    bool DrawTalkPage(NPC* npc, std::size_t talkIndex, int yamlIndex, bool isTutorialTrigger, int proximityMessageMode,
                      const std::vector<StageActorInstance>& talkFocusCandidates);
    void DrawTalkProximityMessage(NPC* npc, std::size_t talkIndex, int yamlIndex, int proximityMessageMode);
    void DrawTalkCameraFocus(NPC* npc, std::size_t talkIndex, int yamlIndex,
                             const std::vector<StageActorInstance>& talkFocusCandidates);

    DebugEditorContext& mContext;
    StageActorAssetEditor& mAssetEditor;
    std::string mRubyGenerationStatus;
};
