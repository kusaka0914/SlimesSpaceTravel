#pragma once

#include "actor/player/PlayerTypes.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <memory>
#include <string>
#include <vector>

class Actor;
class Player;
class Boat;
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
class StageProgressSystem;

class Game {
public:
    Game();
    ~Game();

    bool Initialize(bool isDebugMode);
    void RunLoop();
    void Shutdown();

    void LoadData(bool isLoadPlayer);
    void ReloadCurrentStage();
    void ReloadUIData();
    void ChangeStage(int stageNum);
    bool DebugChangeStage(int stageNum, const std::string& yamlPath);
    void TogglePauseMenu();
    void ClosePauseMenu();
    void MovePauseMenuSelection(int delta);
    void ExecutePauseMenuItem();
    void OpenFeedbackForm();
    void ReturnToBase();
    void TryCreatePlayer2();
    void ToggleDebugEditor();
    void ToggleFreeCameraMode();
    void TogglePlayerControlStyle();

    void OnBoatStageChangeRequested(int destStage);
    void OnBoatArrived(Boat* boat);
    void OnStarObtained();
    void OnEnemyLaunched();
    void OnLanded();
    void OnPlayerDied();
    void OnBoatPartsObtained();
    void OnPlayerApplyDamage(int playerNum);
    void OnPlayerAttackHit(int playerNum);
    void OnStrongAttacked(int playerNum);
    void OnPlayerCounter(int playerNum);
    void VibrateControllerForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration);

    Player* FindNearestPlayer(Actor* actor) const;

    void FinishGame();
    void RestartGame();
    void StartPlayingScene();
    void StartFocusingScene();

    void AddActor(std::unique_ptr<Actor> actor);
    void RemoveActor(Actor* actor);
    void RemoveAllActor();

    void AddPlayer(Player* player);
    void RemoveAllPlayer();
    bool SwitchControlledPlayer();

    void SetHitStopTimer(float hitStopTimer) { mHitStopTimer = hitStopTimer; }
    void SetGroundNormalRayLength(float rayLength)
    {
        mGroundNormalRayLength = rayLength > 0.01f ? rayLength : 0.01f;
    }

    GLFWwindow* GetWindow() const { return mWindow; }
    SDL_GameController* GetSdlController() const;

    const std::vector<Player*>& GetPlayers() const;
    Player* GetMainPlayer() const;
    Player* GetControlledPlayer() const;
    int GetControlledPlayerIndex() const { return mControlledPlayerIndex; }

    const std::vector<Stage*>& GetStages() const;
    Stage* GetCurrentStage() const;
    int GetCurrentStageNum() const;
    const std::string& GetCurrentStageYamlPath() const;
    bool GetIsDebugEditorShowing() const { return mIsDebugEditorShowing; }
    bool GetIsFreeCameraMode() const { return mIsFreeCameraMode; }
    bool GetIsPauseMenuOpen() const;
    int GetPauseMenuSelectedIndex() const;

    AudioSystem* GetAudioSystem() const { return mAudioSystem.get(); }
    PhysicsSystem* GetPhysicsSystem() const { return mPhysicsSystem.get(); }
    MeshLoadSystem* GetMeshLoadSystem() const { return mMeshLoadSystem.get(); }
    SceneSystem* GetSceneSystem() const { return mSceneSystem.get(); }
    ActorLoadSystem* GetActorLoadSystem() const { return mActorLoadSystem.get(); }
    CameraSystem* GetCameraSystem() const { return mCameraSystem.get(); }
    MathUtils* GetMathUtils() const { return mMathUtils.get(); }
    ParticleSystem* GetParticleSystem() const { return mParticleSystem.get(); }
    UIRenderer* GetUIRenderer() const { return mUIRenderer.get(); }
    Renderer3D* GetRenderer3D() const { return mRenderer3D.get(); }
    SequenceSystem* GetSequenceSystem() const { return mSequenceSystem.get(); }

    float GetHitStopTimer() const { return mHitStopTimer; }
    float GetGroundNormalRayLength() const { return mGroundNormalRayLength; }
    bool GetIsPlayer2Joined() const { return mIsPlayer2Joined; }
    bool GetIsDebugMode() const { return mIsDebugMode; }
    PlayerControlStyle GetPlayerControlStyle() const { return mPlayerControlStyle; }
    bool IsAssistControlStyle() const { return mPlayerControlStyle == PlayerControlStyle::Assist; }

    bool IsInBase() const;
    bool IsStageCleared(int stageNum) const;
    void MarkStageCleared(int stageNum);
    void SetStageCleared(int stageNum, bool isCleared);
    bool IsGameControllerConnected() const;

private:
    bool InitializeGLFW();
    void InitializeGameController();
    void CreateGameSystems();
    void CreateStages(int stageCount);

    void ProcessInput();
    void ProcessActorsInput();

    void UpdateGame();
    void UpdateActors(float deltaTime);

    void GenerateOutput();

    void CreatePlayer2();
    void CheckGameControllerConnected();

private:
    GLFWwindow* mWindow = nullptr;

    std::unique_ptr<GameWorld> mWorld;
    std::unique_ptr<PauseMenuController> mPauseMenuController;
    std::unique_ptr<StageFlowController> mStageFlowController;
    std::unique_ptr<GamepadRumbleService> mGamepadRumbleService;

    std::unique_ptr<AudioSystem> mAudioSystem;
    std::unique_ptr<UIRenderer> mUIRenderer;
    std::unique_ptr<Renderer3D> mRenderer3D;
    std::unique_ptr<PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<CameraSystem> mCameraSystem;
    std::unique_ptr<ActorLoadSystem> mActorLoadSystem;
    std::unique_ptr<MeshLoadSystem> mMeshLoadSystem;
    std::unique_ptr<MathUtils> mMathUtils;
    std::unique_ptr<SceneSystem> mSceneSystem;
    std::unique_ptr<InputSystem> mInputSystem;
    std::unique_ptr<ParticleSystem> mParticleSystem;
    std::unique_ptr<SequenceSystem> mSequenceSystem;
    std::unique_ptr<StageProgressSystem> mStageProgressSystem;

    float mHitStopTimer = -1.0f;
    float mGroundNormalRayLength = 5.0f;

    double mLastTime = 0.0;

    bool mIsPlayer2Joined = false;
    int mControlledPlayerIndex = 0;
    bool mIsDebugEditorShowing = false;
    bool mIsFreeCameraMode = false;
    bool mIsDebugMode = false;

    PlayerControlStyle mPlayerControlStyle = PlayerControlStyle::Standard;
};
