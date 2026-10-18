#pragma once

#include <functional>
#include <memory>

class AudioSystem;
class GameProgressController;
class GameWorld;
class InputSystem;
class PauseMenuController;
class PlayerConfigurationController;
class SceneSystem;
class SequenceSystem;
class TgsExperienceController;
class UGCModeController;
enum class PlayerControlStyle;

struct GameSessionDependencies {
    const GameWorld& world;
    SceneSystem& sceneSystem;
    SequenceSystem& sequenceSystem;
    PlayerConfigurationController& playerConfigurationController;
    GameProgressController& progressController;
    PauseMenuController& pauseMenuController;
    InputSystem& inputSystem;
    // タイトル復帰時に同期して呼ぶ。通知先はセッションの寿命中、有効であること。
    std::function<void()> suppressOneShotInput;
    AudioSystem& audioSystem;
    UGCModeController& ugcModeController;
    PlayerControlStyle& playerControlStyle;
};

// タイトルの選択・復帰と、展示版セッションの入力・時間制限・終了遷移を調整する。
// 制限時間やログの状態はTgsExperienceController、シーンの演出はSceneSystemへ委譲する。
class GameSessionController {
public:
    // 依存先は借用し、このControllerより長く生存すること。展示版だけ制限時間とログを有効にする。
    GameSessionController(const GameSessionDependencies& dependencies, bool isTgsBuild);
    ~GameSessionController();

    // InputSystemのフレーム入力取得後に呼ぶ。終了画面では決定キーの押し始めだけを受け付ける。
    void ProcessInput();

    // ポーズ・ヒットストップ中も呼び、実経過秒数で展示版セッションを進める。
    void Update(float elapsedSeconds);

    // タイトル待機中だけ選択・実行を受け付け、シーン遷移中は無視する。
    void MoveTitleMenuSelection(int delta);
    void ExecuteTitleMenuSelection();
    void ResetTitleMenuSelection();
    int GetTitleMenuSelection() const { return mTitleMenuSelection; }

    // 進行と途中終了ログを保存し、フェード経由でタイトルへ戻す。UGCでは編集モードの終了へ委譲する。
    void ReturnToTitle();
    void RecordPlayerDeath();
    void RecordStageCleared(int stageNum);
    void SkipTgsExperienceToRemainingTimeNotice();
    void EndTgsExperienceNow();
    bool IsTgsThankYouScreenVisible() const;
    bool IsTgsRemainingTimeNoticeVisible() const;
    bool ShouldBlockInputForTgsPresentation() const;

private:
    void EnterTitleAtFadeMidpoint();

    const GameWorld& mWorld;
    SceneSystem& mSceneSystem;
    SequenceSystem& mSequenceSystem;
    PlayerConfigurationController& mPlayerConfigurationController;
    GameProgressController& mProgressController;
    PauseMenuController& mPauseMenuController;
    InputSystem& mInputSystem;
    std::function<void()> mSuppressOneShotInput;
    AudioSystem& mAudioSystem;
    UGCModeController& mUGCModeController;
    PlayerControlStyle& mPlayerControlStyle;
    std::unique_ptr<TgsExperienceController> mTgsExperienceController;
    bool mWasTgsReturnConfirmPressed = false;
    int mTitleMenuSelection = 0;
};
