#pragma once

#include "editor/core/DebugPanel.h"

#include <array>
#include <string>

class CameraSystem;
class CinematicSequenceLibrary;

// カメラシーケンスとキーフレームを編集し、実行中のCameraSystemでプレビューする。
class CinematicSequenceDebugPanel : public DebugPanel {
public:
    explicit CinematicSequenceDebugPanel(DebugEditorContext& context);

    // UI入力に応じて共有ライブラリとカメラを更新する。キーフレーム選択でも視点を移動し、フリーカメラを有効にする。
    void Draw() override;

private:
    void DrawSequenceList(CameraSystem* cameraSystem, CinematicSequenceLibrary& library);
    void DrawSequenceEditor(CameraSystem* cameraSystem, CinematicSequenceLibrary& library);
    void SelectSequence(const std::string& sequenceId);
    void SelectKeyframe(int keyframeIndex);

    std::array<char, 128> mNewSequenceIdBuffer = {"new_sequence"};
    std::array<char, 128> mRenameSequenceIdBuffer = {};

    std::string mSelectedSequenceId;
    std::string mStatusMessage;

    int mSelectedKeyframeIndex = -1;
    int mEasingIndex = 3;
    int mTransitionModeIndex = 0;

    float mKeyframeTime = 0.0f;
    float mKeyframeHoldDurationSeconds = 0.0f;
};
