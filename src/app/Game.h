#pragma once

#include "actor/player/PlayerTypes.h"
#include "app/GameWindow.h"
#include "gfx/performance/FramePerformanceTracker.h"
#include "state/ProgressUpdateResult.h"
#include "system/StageFlowController.h"
#include "system/UGCModeRuntime.h"

#include <SDL.h>
#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Actor;
class Enemy;
class JewelItem;
class NPC;
class Player;
class Boat;
class Planet;
class Stage;
class PhysicsSystem;
class MeshLoadSystem;
class ActorLoadSystem;
class CameraSystem;
class MathUtils;
class Renderer3D;
struct SceneRenderState;
class UIRenderer;
class AudioSystem;
class SceneSystem;
class InputSystem;
class GameWorld;
class PauseMenuController;
class StageFlowController;
class GamepadRumbleService;
class ParticleSystem;
class SequenceSystem;
class EnemyJewelDropSystem;
class GameFrameRenderer;
class UGCModeController;
class PlayerConfigurationController;
class GameProgressController;
class UGCPreviewController;
class DebugEditorSessionController;
class UIDebugEditorBridge;
class GameSessionController;
class GameInputController;

enum class InputDeviceType {
    KeyboardMouse,
    GameController,
};

// ゲームのライフサイクルと各System間の連携を統括する。実行中のステージと所属ActorはGameWorldが所有する。
// 実行時APIはInitialize成功後に呼ぶ。返却ポインタは非所有で、Actor・Stageは再読み込みで無効になり得る。
class Game : public UGCModeRuntime {
public:
    Game();
    ~Game();

    // ウィンドウ・描画Systemの初期化に失敗した場合は false。成功後にステージを読み込んで実行準備を行う。
    // areDebugToolsEnabled はツールの利用可否、shouldStartInDebugStage はテストステージ・ウィンドウ起動を指定する。
    bool Initialize(bool areDebugToolsEnabled, bool shouldStartInDebugStage, const std::string& editorSessionPath = {},
                    const std::string& editorRestartErrorLogPath = {});

    // Initialize成功後に呼び、終了要求まで入力・更新・描画を繰り返す。終了後はShutdownを呼ぶ。
    // onFrameCompletedは更新・描画後、FPS制限の待機前に同期呼出する。計測値の参照は呼出中だけ有効。
    void RunLoop(const std::function<void(const FramePerformanceMetrics&)>& onFrameCompleted = {});

    // エディター状態を保存し、描画資源・入力デバイス・ウィンドウを解放する。デストラクタからも呼ぶ。
    // 初期化途中や解放済みの状態でも呼び出せる。
    void Shutdown();

    // 演出と旧Actorへの参照を解除してステージを再読み込みする。返却済みのActor・Stageポインタは取得し直す。
    // 読込失敗時はfalseで旧Actorを保持するが、演出・UI参照は解除済み。SkipRebuildの成功時は呼出元で物理を構築する。
    bool ReloadCurrentStage(StagePhysicsReloadMode physicsReloadMode);

    // 物理ワールドの再構築を含めて現在のステージを再読み込みする。
    bool ReloadCurrentStage() override { return ReloadCurrentStage(StagePhysicsReloadMode::Rebuild); }

    // UI定義を読み直す。UIRendererがなければ何もしない。
    void ReloadUIData();

    // 有効なステージ番号なら選択先とYAMLパスを切り替える。Actorの読み込みは別途行う。
    void ChangeStage(int stageNum);
    bool HasStageIntroCinematic(int stageNum) const;

    // enter_stageNのカメラ演出と導入BGMを開始する。定義不足や再生失敗時は false。
    bool StartStageIntroCinematic(int stageNum);

    // 指定YAMLを再読み込みし、対象パスや演出定義などの条件を満たす場合に到着・導入演出を開始する。
    // 番号が範囲外またはパスが空なら false。
    bool DebugChangeStage(int stageNum, const std::string& yamlPath);

    // デバッグモードで初期ステージを読み直し、タイトルへ切り替える。準備できない場合は false。
    bool DebugEnterTitle();

    // 初期ステージを読み直してオープニングへ切り替える。デバッグモードでなければ false。
    bool DebugEnterOpening();

    // 初期ステージを読み直してエンディングへ切り替える。デバッグモードでなければ false。
    bool DebugEnterEnding();

    // 初期ステージを読み直してエンドロールへ切り替える。デバッグモードでなければ false。
    bool DebugStartCredits();

    // 作業用ステージを読み込んでUGC編集を開始する。ステージの切り替えを受け付けられない場合は false。
    bool StartUGCMode();

    // 練習用ステージをテンプレートから作り直して開始する。準備できない場合は false。
    bool StartUGCEditorTutorial();

    // 完了・スキップの進行フラグを記録し、通常のUGC編集へ移る。通常編集を開始できなければ false。
    bool FinishUGCEditorTutorial(bool wasCompleted);
    bool GetIsUGCEditorTutorialActive() const;
    void CloseUGCWorkBrowser();
    int GetTitleMenuSelection() const;
    bool GetIsUGCWorkBrowserShowing() const;

    // 編集中ステージを通常の試遊として開始する。クリア検証はStartUGCClearVerificationで開始する。
    void StartUGCPlaytest();
    void StartUGCSavedWorkPlaytest();

    // 指定した保存作品のクリア検証を開始する。作品名はUGC作品管理で使うファイル名を渡す。
    void StartUGCClearVerification(const std::string& workFileName);

    // フェード経由のUGC終了を要求する。終了するまで通常の更新を続ける必要がある。
    void ExitUGCMode();
    bool GetIsUGCClearResultShowing() const;
    int GetUGCClearResultSelection() const;

    // エディター復元用に指定ステージを読み直す。導入演出は開始せず、不正な番号・空パスには false。
    bool RestoreDebugEditorStage(int stageNum, const std::string& yamlPath);
    void TogglePauseMenu();
    void ClosePauseMenu() override;
    bool CanOpenPauseMenu() const;
    void MovePauseMenuSelection(int delta);
    void ExecutePauseMenuItem();

    // 外部ブラウザーでフィードバックフォームを開く。起動失敗は標準エラー出力へ記録する。
    void OpenFeedbackForm();

    // ポーズメニューを閉じ、拠点へのシーン遷移を要求する。既に拠点なら遷移しない。
    void ReturnToBase();

    // 2人プレイへの参加を試みる。参加条件を満たさない場合は現在の構成を維持する。
    void TryCreatePlayer2();
    void ReturnToSinglePlayer();
    bool CanStartTwoPlayerFromPauseMenu() const;
    bool CanReturnToBaseFromPauseMenu() const;

    // エディター表示を切り替える。通常ステージでは物理ボディの位置も同期し、UGCでは専用パネルへ委譲する。
    void ToggleDebugEditor();
    void ToggleFreeCameraMode();
    void SetFreeCameraMode(bool isEnabled) override;
    void SetDebugEditorShowing(bool isShowing) override { mIsDebugEditorShowing = isShowing; }
    void TogglePlayerControlStyle();

    // 操作方式を変更し、進行管理にも選択済みの方式として記録する。
    void SetPlayerControlStyle(PlayerControlStyle controlStyle);

    // ビルド・再起動を要求する。失敗時は false と説明をoutErrorMessageへ返す。
    bool RequestEditorBuildAndRestart(std::string& outErrorMessage);

    // 拠点にいる場合だけ、Boatが指定した移動先ステージへのシーン遷移を要求する。
    void OnBoatStageChangeRequested(int destStage);

    // 到着演出へ通知し、ソロ分身の合体とBGMの切り替えを行う。Boatは所有しない。
    void OnBoatArrived(Boat* boat);
    void OnPlayerCurrentPlanetChanged(Player& player);

    // UGCではモード固有のゴール処理へ委譲し、通常プレイではクリアを記録してクリア演出を開始する。
    void OnStarObtained();
    void OnStarCollectionAnimationFinished();
    void ForcePlayersGroundedForCinematic();
    void OnEnemyLaunched();

    // 敵の配置からドロップを予約する。Actor走査中の生成を避けるため、実体は更新後に生成する。
    void RequestEnemyJewelDrop(const Enemy& defeatedEnemy);
    void OnLanded();
    void OnPlayerDied();
    void OnBoatPartsObtained();
    void OnPlayerApplyDamage(int playerNum);

    // 回復成功を操作案内へ通知し、回復の一時案内を終了する。
    void OnPlayerRecovered(int playerNum);

    void OnPlayerAttackHit(int playerNum);
    void OnStrongAttacked(int playerNum);
    void OnAirSlamAttackHit(int playerNum);
    void OnPlayerCounter(int playerNum);

    // ソロ分身間で共有するリソースをsourcePlayerの状態に合わせる。
    void SynchronizeSoloSplitResources(const Player& sourcePlayer);

    // playerNum は1始まり。低・高周波の強度は0〜65535、時間はミリ秒。対応するデバイスがなければ何もしない。
    void VibrateControllerForPlayer(int playerNum, int lowFrequency, int highFrequency, int durationMilliseconds);

    // 指定Actorに最も近いPlayerを所有権を移さず返す。有効な探索対象がなければ nullptr。
    Player* FindNearestPlayer(Actor* actor) const;

    // 進行を保存してタイトルへの遷移を要求する。UGCでは編集モードを終了する。
    void ReturnToTitleFromPauseMenu();

    // Playerをリスタートしてカメラを戻す。待機中Playerは非アクティブのまま維持する。
    void RestartGame();
    void StartPlayingScene() override;
    void StartUGCStageClearPresentation() override;
    void StartFocusingScene();
    void FinishFocusingScene();

    // 非nullのActorを渡し、その所有権をGameWorldへ移す。
    void AddActor(std::unique_ptr<Actor> actor);

    // 所有権を移さずPlayer一覧へ登録し、現在のプレイ人数・分身構成に合わせて状態を設定する。nullptr は無視する。
    void AddPlayer(Player* player);

    // Playerの参照一覧と構成管理をリセットする。Actorとしての実体は削除しない。
    void RemoveAllPlayer();

    // ソロ分身の分裂・合体を要求する。操作できない状態なら false。
    bool TogglePlayerSplit();
    void SetPlayerSplitMergeButtonHeld(bool isHeld);

    // 合体ガイドの対象とワールド単位の半径を返す。解決できなければ false、対象はnullptr、半径は0。
    bool TryResolvePlayerMergeGuide(const Player*& targetPlayer, float& radiusWorldUnits) const;

    // 非操作側のソロ分身への被弾でガードを消費する。対象外・残数不足なら false。
    bool TryConsumePlayerSplitGuard(const Player& damagedPlayer);
    int GetPlayerSplitGuardCount() const;
    int GetMaximumPlayerSplitGuardCount() const;

    // 操作可能な別の分身へ切り替える。切り替え条件を満たさない場合は false。
    bool SwitchControlledPlayer();
    bool CanTogglePlayerSplit() const;
    bool CanSwitchControlledPlayer() const;
    void RequestSoloSplitControlSwitchAfterBoarding();

    void SetHitStopTimer(float hitStopTimer) { mHitStopTimer = hitStopTimer; }

    // 接地判定レイの長さをワールド単位で設定し、最小0.01に制限する。
    void SetGroundNormalRayLength(float rayLength) { mGroundNormalRayLength = rayLength > 0.01f ? rayLength : 0.01f; }

    // 頭上重力レイの長さをワールド単位で設定し、最小0.01に制限する。
    void SetOverheadGravityRayLength(float rayLength)
    {
        mOverheadGravityRayLength = rayLength > 0.01f ? rayLength : 0.01f;
    }

    GLFWwindow* GetWindow() const { return mWindow.GetHandle(); }
    SDL_GameController* GetSdlController() const;

    const std::vector<Player*>& GetPlayers() const;
    const std::vector<JewelItem*>& GetRuntimeJewelItems() const;
    Player* GetMainPlayer() const;
    Player* GetControlledPlayer() const;
    int GetControlledPlayerIndex() const;

    const std::vector<Stage*>& GetStages() const;
    Stage* GetCurrentStage() const;
    int GetCurrentStageNum() const;
    const std::string& GetCurrentStageYamlPath() const;

    // シーン遷移用にステージとYAMLパスを切り替えて再読み込みする。番号が範囲外またはパスが空なら false。
    bool LoadStageForScene(int stageNum, const std::string& yamlPath);
    std::string GetNPCConversationId(const NPC* npc) const;

    // 現在のステージ内を会話IDで検索する。空ID・対象なしには nullptr。返却NPCは所有しない。
    NPC* FindNPCByConversationId(const std::string& conversationId) const;
    bool GetIsDebugEditorShowing() const override { return mIsDebugEditorShowing; }
    void SetGameUIHidden(bool isHidden) { mIsGameUIHidden = isHidden; }
    bool GetIsGameUIHidden() const { return mIsGameUIHidden; }
    bool GetIsUGCMode() const;
    bool GetIsUGCPlaytestActive() const;
    bool GetIsUGCClearVerificationActive() const;

    bool GetIsUGCDebugEditorShowing() const;
    bool GetIsUGCOrthographicView() const;
    void SetIsUGCOrthographicView(bool isOrthographic);
    void SetUGCOrthographicHalfHeight(float halfHeight) override;
    float GetUGCOrthographicHalfHeight() const;
    float GetUGCGridSize() const;

    // プレビュー描画用のOpenGLテクスチャIDを返す。描画担当が未生成なら0。
    unsigned int GetUGCPreviewTexture() const;

    // 描画サイズをピクセル単位で指定し、幅320〜2048・高さ180〜1152に制限する。
    void SetUGCPreviewRenderSize(int width, int height);

    // プレビュー視点のヨー角へラジアン単位の差分を加える。
    void AdjustUGCPreviewYaw(float yawDeltaRadians);
    float GetUGCPreviewYawRadians() const;
    void ToggleUGCPreviewVerticalView();
    bool GetIsUGCPreviewViewedFromBelow() const;
    float GetUGCPreviewFocusY() const;
    void SetUGCPreviewEditLayer(int gridLayer);
    int GetUGCPreviewEditLayer() const;

    // ワールド座標で足場の配置候補を設定する。std::nullopt なら表示を解除する。
    void SetUGCPlatformPlacementPreview(const std::optional<glm::vec3>& position);

    // 移動足場の両端をワールド座標で指定する。未指定の端点はプレビューから除く。
    void SetUGCMovingPlatformPathPreview(const std::optional<glm::vec3>& startPosition,
                                         const std::optional<glm::vec3>& destinationPosition);

    // ワールド座標で配置用モデルを設定する。位置未指定・空モデル名なら破棄し、モデル変更時は作り直す。
    void SetUGCPlacementModelPreview(const std::optional<glm::vec3>& position, const std::string& modelPath = "",
                                     const glm::vec3& scale = glm::vec3(1.0f),
                                     const std::string& textureOverridePath = "");

    // 1つの配置用モデルを複数のワールド位置へ表示する。位置一覧が空ならモデルを破棄する。
    void SetUGCPlacementModelPreviewPositions(const std::vector<glm::vec3>& positions, const std::string& modelPath,
                                              const glm::vec3& scale, const std::string& textureOverridePath = "");

    // 直近のゲーム更新に使った秒数。長い停止後も最大0.04秒に制限する。
    float GetLastDeltaTime() const { return mLastDeltaTime; }
    const FramePerformanceMetrics& GetFramePerformanceMetrics() const;

    bool IsEditorKeyboardInputCaptured() const;
    bool GetIsFreeCameraMode() const override { return mIsFreeCameraMode; }
    bool GetIsPauseMenuOpen() const;
    int GetPauseMenuSelectedIndex() const;

    AudioSystem* GetAudioSystem() const { return mAudioSystem.get(); }
    PhysicsSystem* GetPhysicsSystem() const { return mPhysicsSystem.get(); }
    MeshLoadSystem* GetMeshLoadSystem() const { return mMeshLoadSystem.get(); }
    SceneSystem* GetSceneSystem() const { return mSceneSystem.get(); }
    InputSystem* GetInputSystem() const { return mInputSystem.get(); }
    ActorLoadSystem* GetActorLoadSystem() const { return mActorLoadSystem.get(); }
    CameraSystem* GetCameraSystem() const { return mCameraSystem.get(); }
    MathUtils* GetMathUtils() const { return mMathUtils.get(); }
    ParticleSystem* GetParticleSystem() const { return mParticleSystem.get(); }
    UIRenderer* GetUIRenderer() const { return mUIRenderer.get(); }
    Renderer3D* GetRenderer3D() const { return mRenderer3D.get(); }
    SequenceSystem* GetSequenceSystem() const { return mSequenceSystem.get(); }

    float GetGroundNormalRayLength() const { return mGroundNormalRayLength; }
    float GetOverheadGravityRayLength() const { return mOverheadGravityRayLength; }
    bool GetIsPlayer2Joined() const;
    bool GetIsPlayerSplit() const;
    bool GetIsDebugMode() const { return mIsDebugMode; }
    bool IsReviewBuild() const;
    bool IsTgsBuild() const;
    bool IsTgsThankYouScreenVisible() const;
    bool IsTgsRemainingTimeNoticeVisible() const;
    bool ShouldBlockInputForTgsPresentation() const;
    PlayerControlStyle GetPlayerControlStyle() const { return mPlayerControlStyle; }
    bool IsAssistControlStyle() const { return mPlayerControlStyle == PlayerControlStyle::Assist; }
    bool HasSelectedPlayerControlStyle() const;

    bool IsInBase() const;
    bool IsStageCleared(int stageNum) const;
    ProgressUpdateResult MarkStageCleared(int stageNum);

    // 変更時はActor表示と物理を更新し、クリア時は体験版ログへ記録する。進行の変更と保存の結果を返す。
    ProgressUpdateResult SetStageCleared(int stageNum, bool isCleared);
    bool HasCompletedTutorial(const std::string& tutorialId) const;
    void MarkTutorialCompleted(const std::string& tutorialId);
    bool HasShownNPCConversation(const NPC* npc) const;
    void MarkNPCConversationShown(const NPC* npc);
    bool HasSeenBaseIntro() const;
    void MarkBaseIntroSeen();
    bool HasCompletedNPCOpeningTrigger(const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCOpeningTriggerCompleted(const NPC* npc, std::size_t talkPageIndex);
    bool AreAllMainStagesCleared() const;
    bool HasCompletedEndingRoll() const;
    void MarkEndingRollCompleted();
    bool HasCompletedNPCEndingTrigger(const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCEndingTriggerCompleted(const NPC* npc, std::size_t talkPageIndex);
    bool IsGameControllerConnected() const;
    bool HasGameControllerForPlayer(int playerNum) const;
    InputDeviceType GetLastUsedInputDevice() const { return mLastUsedInputDevice; }
    void RecordInputDeviceUsage(InputDeviceType inputDevice) { mLastUsedInputDevice = inputDevice; }

private:
    void InitializeGameController();
    bool CreateGameSystems();
    void CreateStages(int stageCount);

    void ProcessInput();
    void ProcessActorsInput();
    void UpdateGame();
    void UpdateWorldAndCamera(float deltaTime);
    SceneRenderState CreateSceneRenderState() const;
    void UpdateActors(float deltaTime);

    void ProcessPendingUGCClearCompletion();

    void CheckGameControllerConnected();
    void UpdatePendingSoloSplitControlSwitch(float deltaTime);
    bool LoadDebugStage(int stageNum, const std::string& yamlPath) override;
    bool IsTitleScene() const override;
    void SetDebugCameraPose(const CameraPose& pose) override;
    bool RequestSceneFadeAction(const std::function<void()>& fadeAction) override;
    void NotifyUGCTutorialReturnedFromPlaytest() override;
    void CompleteUGCVerification(const std::string& workFileName) override;
    bool HasProgressFlag(const std::string& progressId) const override;
    void MarkProgressFlag(const std::string& progressId) override;
    void EnterTitleAtFadeMidpoint() override;
    void TryChangeBGM() override;
    bool PrepareInitialSceneForDebug();
    void RestoreDebugEditorSessionAtStartup(const std::string& editorSessionPath,
                                            const std::string& editorRestartErrorLogPath);
    void SavePersistentDebugEditorSession();

    GameWindow mWindow;

    std::unique_ptr<GameWorld> mWorld;
    std::unique_ptr<PauseMenuController> mPauseMenuController;
    std::unique_ptr<StageFlowController> mStageFlowController;
    std::unique_ptr<GamepadRumbleService> mGamepadRumbleService;

    std::unique_ptr<AudioSystem> mAudioSystem;
    std::unique_ptr<UIRenderer> mUIRenderer;
    std::unique_ptr<UIDebugEditorBridge> mEditor;
    std::unique_ptr<Renderer3D> mRenderer3D;
    std::unique_ptr<GameFrameRenderer> mFrameRenderer;
    std::unique_ptr<UGCModeController> mUGCModeController;
    std::unique_ptr<PlayerConfigurationController> mPlayerConfigurationController;
    std::unique_ptr<GameProgressController> mProgressController;
    std::unique_ptr<UGCPreviewController> mUGCPreviewController;
    std::unique_ptr<DebugEditorSessionController> mDebugEditorSessionController;
    std::unique_ptr<PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<SceneSystem> mSceneSystem;
    std::unique_ptr<InputSystem> mInputSystem;
    std::unique_ptr<CameraSystem> mCameraSystem;
    std::unique_ptr<ActorLoadSystem> mActorLoadSystem;
    std::unique_ptr<MeshLoadSystem> mMeshLoadSystem;
    std::unique_ptr<MathUtils> mMathUtils;
    std::unique_ptr<GameInputController> mGameInputController;
    std::unique_ptr<ParticleSystem> mParticleSystem;
    std::unique_ptr<SequenceSystem> mSequenceSystem;
    std::unique_ptr<EnemyJewelDropSystem> mEnemyJewelDropSystem;

    float mHitStopTimer = -1.0f;
    float mGroundNormalRayLength = 5.0f;
    float mOverheadGravityRayLength = 15.0f;

    double mLastTime = 0.0;
    float mLastDeltaTime = 0.0f;
    FramePerformanceTracker mFramePerformanceTracker;

    bool mIsDebugEditorShowing = false;
    bool mIsGameUIHidden = false;
    bool mIsFreeCameraMode = false;
    bool mIsDebugMode = false;

    PlayerControlStyle mPlayerControlStyle = PlayerControlStyle::Standard;
    std::unique_ptr<GameSessionController> mSessionController;
    InputDeviceType mLastUsedInputDevice = InputDeviceType::KeyboardMouse;
};
