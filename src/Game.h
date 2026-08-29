#pragma once

#include "actor/player/PlayerTypes.h"
#include "gfx/performance/FramePerformanceTracker.h"
#include "system/UGCModeRuntime.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
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

enum class InputDeviceType {
    KeyboardMouse,
    GameController,
};

enum class StagePhysicsReloadMode {
    Rebuild,
    SkipRebuild,
};

class Game : public UGCModeRuntime {
public:
    Game();
    ~Game();

    bool Initialize(
        bool isDebugMode,
        const std::string& editorSessionPath = {},
        const std::string& editorRestartErrorLogPath = {});
    void RunLoop();
    void Shutdown();

    void LoadData();


    void ReloadCurrentStage(
        StagePhysicsReloadMode physicsReloadMode);
    void ReloadCurrentStage() override
    {
        ReloadCurrentStage(StagePhysicsReloadMode::Rebuild);
    }
    void ReloadUIData();
    void ChangeStage(int stageNum);
    bool HasStageIntroCinematic(int stageNum) const;
    bool StartStageIntroCinematic(int stageNum);
    bool DebugChangeStage(int stageNum, const std::string& yamlPath);
    bool DebugEnterTitle();
    bool DebugEnterOpening();
    bool DebugEnterEnding();
    bool DebugStartCredits();
    bool StartUGCMode();
    bool StartUGCEditorTutorial();
    bool FinishUGCEditorTutorial(bool wasCompleted);
    bool GetIsUGCEditorTutorialActive() const;
    void OpenUGCWorkBrowser();
    void CloseUGCWorkBrowser();
    void MoveTitleMenuSelection(int delta);
    void ExecuteTitleMenuSelection();
    int GetTitleMenuSelection() const { return mTitleMenuSelection; }
    bool GetIsUGCWorkBrowserShowing() const;
    void StartUGCPlaytest();
    void UndoUGCEdit();
    void RedoUGCEdit();
    void ToggleUGCEraser();
    void SelectUGCEditorMode();
    void OpenUGCEditorMenu();
    void ZoomUGCEditor(float distanceMultiplier);
    void ChangeUGCEditLayer(int layerDelta);
    void MoveUGCSelectionByGrid(int gridX, int gridZ);
    void StartUGCClearVerification(const std::string& workFileName);
    void ReturnToUGCEditor();
    void ExitUGCMode();
    bool RestoreDebugEditorStage(int stageNum, const std::string& yamlPath);
    void TogglePauseMenu();
    void ClosePauseMenu() override;
    bool CanOpenPauseMenu() const;
    void MovePauseMenuSelection(int delta);
    void ExecutePauseMenuItem();
    void OpenFeedbackForm();
    void ReturnToBase();
    void TryCreatePlayer2();
    void ReturnToSinglePlayer();
    bool CanStartTwoPlayerFromPauseMenu() const;
    bool CanReturnToBaseFromPauseMenu() const;
    void ToggleDebugEditor();
    void ToggleFreeCameraMode();
    void SetFreeCameraMode(bool isEnabled) override;
    void SetDebugEditorShowing(bool isShowing) override
    {
        mIsDebugEditorShowing = isShowing;
    }
    void TogglePlayerControlStyle();
    void SetPlayerControlStyle(PlayerControlStyle controlStyle);
    bool RequestEditorBuildAndRestart(std::string& outErrorMessage);

    void OnBoatStageChangeRequested(int destStage);
    void OnBoatArrived(Boat* boat);
    void OnPlayerCurrentPlanetChanged(Player& player);
    void OnStarObtained();
    void ForcePlayersGroundedForCinematic();
    void OnEnemyLaunched();
    void RequestEnemyJewelDrop(const Enemy& defeatedEnemy);
    void OnLanded();
    void OnPlayerDied();
    void OnBoatPartsObtained();
    void OnPlayerApplyDamage(int playerNum);
    void OnPlayerAttackHit(int playerNum);
    void OnStrongAttacked(int playerNum);
    void OnPlayerCounter(int playerNum);
    void SynchronizeSoloSplitResources(const Player& sourcePlayer);
    void VibrateControllerForPlayer(
        int playerNum,
        int lowFrequency,
        int highFrequency,
        int durationMilliseconds);

    Player* FindNearestPlayer(Actor* actor) const;

    void FinishGame();
    void RestartGame();
    void StartPlayingScene() override;
    void StartFocusingScene();

    void AddActor(std::unique_ptr<Actor> actor);
    void AddPlayer(Player* player);
    void RemoveAllPlayer();
    bool TogglePlayerSplit();
    bool SwitchControlledPlayer();
    bool CanTogglePlayerSplit() const;
    bool CanSwitchControlledPlayer() const;
    void RequestSoloSplitControlSwitchAfterBoarding();

    void SetHitStopTimer(float hitStopTimer) { mHitStopTimer = hitStopTimer; }
    void SetGroundNormalRayLength(float rayLength)
    {
        mGroundNormalRayLength = rayLength > 0.01f ? rayLength : 0.01f;
    }
    void SetOverheadGravityRayLength(float rayLength)
    {
        mOverheadGravityRayLength =
            rayLength > 0.01f ? rayLength : 0.01f;
    }

    GLFWwindow* GetWindow() const { return mWindow; }
    SDL_GameController* GetSdlController() const;
    SDL_GameController* GetSdlControllerForPlayer(int playerNum) const;

    const std::vector<Player*>& GetPlayers() const;
    const std::vector<JewelItem*>& GetRuntimeJewelItems() const;
    Player* GetMainPlayer() const;
    Player* GetControlledPlayer() const;
    int GetControlledPlayerIndex() const;

    const std::vector<Stage*>& GetStages() const;
    Stage* GetCurrentStage() const;
    int GetCurrentStageNum() const;
    const std::string& GetCurrentStageYamlPath() const;
    bool LoadStageForScene(int stageNum, const std::string& yamlPath);
    std::string GetNPCConversationId(const NPC* npc) const;
    NPC* FindNPCByConversationId(const std::string& conversationId) const;
    bool GetIsDebugEditorShowing() const { return mIsDebugEditorShowing; }
    bool GetIsUGCMode() const;
    bool GetIsUGCPlaytestActive() const;
    bool GetIsUGCClearVerificationActive() const;


    bool GetIsUGCDebugEditorShowing() const;
    bool GetIsUGCOrthographicView() const;
    void SetIsUGCOrthographicView(bool isOrthographic);
    float GetUGCOrthographicHalfHeight() const;
    unsigned int GetUGCPreviewTexture() const;
    void SetUGCPreviewRenderSize(int width, int height);
    void AdjustUGCPreviewYaw(float yawDeltaRadians);
    float GetUGCPreviewYawRadians() const;
    void ToggleUGCPreviewVerticalView();
    bool GetIsUGCPreviewViewedFromBelow() const;
    float GetUGCPreviewFocusY() const;
    void SetUGCPreviewEditLayer(int gridLayer);
    int GetUGCPreviewEditLayer() const;
    void SetUGCPlatformPlacementPreview(
        const std::optional<glm::vec3>& position);
    const std::optional<glm::vec3>& GetUGCPlatformPlacementPreview() const;
    void SetUGCMovingPlatformPathPreview(
        const std::optional<glm::vec3>& startPosition,
        const std::optional<glm::vec3>& destinationPosition);
    const std::optional<glm::vec3>&
        GetUGCMovingPlatformPathStartPosition() const;
    const std::optional<glm::vec3>&
        GetUGCMovingPlatformPathDestinationPosition() const;
    void SetUGCPlacementModelPreview(
        const std::optional<glm::vec3>& position,
        const std::string& modelPath = "",
        const glm::vec3& scale = glm::vec3(1.0f),
        const std::string& textureOverridePath = "");
    void SetUGCPlacementModelPreviewPositions(
        const std::vector<glm::vec3>& positions,
        const std::string& modelPath,
        const glm::vec3& scale,
        const std::string& textureOverridePath = "");
    Actor* GetUGCPlacementModelPreview() const;
    const std::vector<glm::vec3>&
        GetUGCPlacementModelPreviewPositions() const;
    void SetUGCOrthographicHalfHeight(float halfHeight) override;
    float GetUGCGridSize() const;
    float GetLastDeltaTime() const { return mLastDeltaTime; }
    const FramePerformanceMetrics& GetFramePerformanceMetrics() const;
    void RecordViewportRenderDurationMilliseconds(
        int viewportIndex,
        float durationMilliseconds);
    void RecordViewportGpuDurationMilliseconds(
        int viewportIndex,
        float durationMilliseconds);
    void SetUGCGridSize(float gridSize);
    bool IsEditorKeyboardInputCaptured() const;
    bool GetIsFreeCameraMode() const { return mIsFreeCameraMode; }
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

    float GetHitStopTimer() const { return mHitStopTimer; }
    float GetGroundNormalRayLength() const { return mGroundNormalRayLength; }
    float GetOverheadGravityRayLength() const
    {
        return mOverheadGravityRayLength;
    }
    bool GetIsPlayer2Joined() const;
    bool GetIsPlayerSplit() const;
    bool GetIsDebugMode() const { return mIsDebugMode; }
    PlayerControlStyle GetPlayerControlStyle() const { return mPlayerControlStyle; }
    bool IsAssistControlStyle() const { return mPlayerControlStyle == PlayerControlStyle::Assist; }
    bool HasSelectedPlayerControlStyle() const;

    bool IsInBase() const;
    bool IsStageCleared(int stageNum) const;
    void MarkStageCleared(int stageNum);
    void SetStageCleared(int stageNum, bool isCleared);
    bool HasCompletedTutorial(const std::string& tutorialId) const;
    void MarkTutorialCompleted(const std::string& tutorialId);
    bool HasShownNPCConversation(const NPC* npc) const;
    void MarkNPCConversationShown(const NPC* npc);
    bool HasSeenBaseIntro() const;
    void MarkBaseIntroSeen();
    bool HasCompletedNPCOpeningTrigger(
        const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCOpeningTriggerCompleted(
        const NPC* npc, std::size_t talkPageIndex);
    bool AreAllMainStagesCleared() const;
    bool HasCompletedEndingRoll() const;
    void MarkEndingRollCompleted();
    bool HasCompletedNPCEndingTrigger(
        const NPC* npc, std::size_t talkPageIndex) const;
    void MarkNPCEndingTriggerCompleted(
        const NPC* npc, std::size_t talkPageIndex);
    bool IsGameControllerConnected() const;
    bool HasGameControllerForPlayer(int playerNum) const;
    InputDeviceType GetLastUsedInputDevice() const { return mLastUsedInputDevice; }
    void RecordInputDeviceUsage(InputDeviceType inputDevice)
    {
        mLastUsedInputDevice = inputDevice;
    }
    bool IsInputModifierHeld() const { return mIsInputModifierHeld; }
    void SetInputModifierHeld(bool isHeld) { mIsInputModifierHeld = isHeld; }

private:
    bool InitializeGLFW();
    void InitializeGameController();
    bool CreateGameSystems();
    void CreateStages(int stageCount);

    void ProcessInput();
    void ProcessActorsInput();
    void UpdateGame();
    void UpdateActors(float deltaTime);

    void ProcessPendingUGCClearCompletion();

    void CheckGameControllerConnected();
    void UpdatePendingSoloSplitControlSwitch(float deltaTime);
    bool LoadDebugStage(
        int stageNum,
        const std::string& yamlPath) override;
    bool IsTitleScene() const override;
    void SetDebugCameraPose(const CameraPose& pose) override;
    bool RequestSceneFadeAction(
        const std::function<void()>& fadeAction) override;
    void NotifyUGCTutorialReturnedFromPlaytest() override;
    void CompleteUGCVerification(
        const std::string& workFileName) override;
    bool HasProgressFlag(const std::string& progressId) const override;
    void MarkProgressFlag(const std::string& progressId) override;
    void EnterTitleAtFadeMidpoint() override;
    void TryChangeBGM() override;
    bool PrepareInitialSceneForDebug();
    void RestoreDebugEditorSessionAtStartup(
        const std::string& editorSessionPath,
        const std::string& editorRestartErrorLogPath);
    void SavePersistentDebugEditorSession();

private:
    GLFWwindow* mWindow = nullptr;

    std::unique_ptr<GameWorld> mWorld;
    std::unique_ptr<PauseMenuController> mPauseMenuController;
    std::unique_ptr<StageFlowController> mStageFlowController;
    std::unique_ptr<GamepadRumbleService> mGamepadRumbleService;

    std::unique_ptr<AudioSystem> mAudioSystem;
    std::unique_ptr<UIRenderer> mUIRenderer;
    std::unique_ptr<Renderer3D> mRenderer3D;
    std::unique_ptr<GameFrameRenderer> mFrameRenderer;
    std::unique_ptr<UGCModeController> mUGCModeController;
    std::unique_ptr<PlayerConfigurationController>
        mPlayerConfigurationController;
    std::unique_ptr<GameProgressController> mProgressController;
    std::unique_ptr<UGCPreviewController> mUGCPreviewController;
    std::unique_ptr<DebugEditorSessionController>
        mDebugEditorSessionController;
    std::unique_ptr<PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<CameraSystem> mCameraSystem;
    std::unique_ptr<ActorLoadSystem> mActorLoadSystem;
    std::unique_ptr<MeshLoadSystem> mMeshLoadSystem;
    std::unique_ptr<MathUtils> mMathUtils;
    std::unique_ptr<SceneSystem> mSceneSystem;
    std::unique_ptr<InputSystem> mInputSystem;
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
    int mTitleMenuSelection = 0;
    bool mIsFreeCameraMode = false;
    bool mIsDebugMode = false;

    PlayerControlStyle mPlayerControlStyle = PlayerControlStyle::Standard;
    InputDeviceType mLastUsedInputDevice = InputDeviceType::KeyboardMouse;
    bool mIsInputModifierHeld = false;

};
