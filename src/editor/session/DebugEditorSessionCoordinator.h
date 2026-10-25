#pragma once

#include <string>

struct DebugEditorContext;
class StageEditorPanel;
class StagePlanetPanel;
class StageSelectionController;

// ワークスペース側で保持するタブと演出メニューの選択状態。
struct DebugEditorShellSessionState {
    int activeSectionIndex = 0;
    int sequenceEditorMenuIndex = 0;
};

// ゲーム・編集パネルからセッションを集め、ステージの再読み込みと状態復元を調整する。
// contextと渡されたPanel・Controllerは非所有参照。
class DebugEditorSessionCoordinator {
public:
    DebugEditorSessionCoordinator(DebugEditorContext& context, StagePlanetPanel& stagePlanetPanel,
                                  StageEditorPanel& stageEditorPanel, StageSelectionController& selectionController);

    // 編集Transformの保存を要求してからセッションを書き出す。戻り値はセッション保存の成否。
    // セッション保存失敗時はfalseとoutErrorMessageを返す。
    bool Save(const std::string& filePath, const DebugEditorShellSessionState& shellState,
              std::string& outErrorMessage);

    // ステージを再読み込みし、画面・カメラ・Player姿勢・選択を復元する。失敗時はfalseとoutErrorMessageを返す。
    bool Restore(const std::string& filePath, DebugEditorShellSessionState& outShellState,
                 std::string& outErrorMessage);

private:
    DebugEditorContext& mContext;
    StagePlanetPanel& mStagePlanetPanel;
    StageEditorPanel& mStageEditorPanel;
    StageSelectionController& mSelectionController;
};
