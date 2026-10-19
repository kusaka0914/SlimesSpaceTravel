#pragma once

#include "editor/core/DebugPanel.h"
#include "system/sequence/SequenceTypes.h"

#include <array>
#include <string>

class Actor;
class SequenceSystem;

// Actor移動・表示・操作制御・カメラ演出のシーケンスを編集し、ゲーム内でプレビューする。
class SequenceDebugPanel : public DebugPanel {
public:
    explicit SequenceDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawSequenceList(SequenceSystem* system);
    void DrawSequenceEditor(SequenceSystem* system);
    void DrawClipList(GameplaySequence& sequence);
    void DrawClipInspector(SequenceSystem* system, GameplaySequence& sequence);

    void DrawActorTargetPicker(SequenceClip& clip);
    void DrawCameraSequencePicker(SequenceClip& clip);
    void AddArrivalTemplate(GameplaySequence& sequence);

    static const char* GetClipTypeLabel(SequenceClipType type);
    static std::string GetClipListLabel(const SequenceClip& clip, int index);

    std::array<char, 128> mNewSequenceId = {"new_sequence"};
    std::array<char, 128> mRenameSequenceId = {};
    std::string mSelectedSequenceId;
    std::string mStatusMessage;
    int mSelectedClipIndex = -1;
    int mNewClipType = 0;
    int mEasingIndex = 3;
};
