#pragma once

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

class Game {
public:
    Game();
    ~Game();

    bool Initialize();
    void RunLoop();
    void Shutdown();

    void LoadData(bool isLoadPlayer);
    void ReloadCurrentStage();
    void ChangeStage(int stageNum);

    void OnBoatStageChangeRequested(int destStage);
    void OnBoatArrived(Boat* boat);
    void OnStarObtained();
    void OnEnemyLaunched();
    void OnPlayerAttackHit();
    void OnStrongAttacked();
    void OnPlayerCounter();
    void OnPlayerApplyDamage();
    void OnPlayerFinishCharging();
    void OnLanded();
    void OnPlayerDied();
    void OnBoatPartsObtained();

    Player* FindNearestPlayer(Actor* actor) const;

    void FinishGame();
    void RestartGame();
    void StartPlayingScene();
    void StartFocusingScene();

    void AddActor(std::unique_ptr<Actor> actor);
    void RemoveActor(Actor* actor);
    void RemoveAllActor();

    void AddPlayer(Player* player) { mPlayers.emplace_back(player); }
    void RemoveAllPlayer() { mPlayers.clear(); }

    void SetHitStopTimer(float hitStopTimer) { mHitStopTimer = hitStopTimer; }

    GLFWwindow* GetWindow() const { return mWindow; }
    SDL_GameController* GetSdlController() const { return mSdlController; }

    const std::vector<Player*>& GetPlayers() const { return mPlayers; }
    Player* GetMainPlayer() const { return mPlayers.empty() ? nullptr : mPlayers[0]; }

    const std::vector<Stage*>& GetStages() const { return mStages; }
    Stage* GetCurrentStage() const { return mCurrentStage; }
    int GetCurrentStageNum() const { return mCurrentStageNum; }
    const std::string& GetCurrentStageYamlPath() const { return mCurrentStageYamlPath; }

    AudioSystem* GetAudioSystem() const { return mAudioSystem.get(); }
    PhysicsSystem* GetPhysicsSystem() const { return mPhysicsSystem.get(); }
    MeshLoadSystem* GetMeshLoadSystem() const { return mMeshLoadSystem.get(); }
    SceneSystem* GetSceneSystem() const { return mSceneSystem.get(); }
    ActorLoadSystem* GetActorLoadSystem() const { return mActorLoadSystem.get(); }
    CameraSystem* GetCameraSystem() const { return mCameraSystem.get(); }
    MathUtils* GetMathUtils() const { return mMathUtils.get(); }

    float GetHitStopTimer() const { return mHitStopTimer; }
    bool GetIsPlayer2Joined() const { return mIsPlayer2Joined; }

    bool IsInBase() const { return mCurrentStageNum == 0; }
    bool IsGameControllerConnected() const { return mSdlController != nullptr; }

private:
    bool InitializeGLFW();
    void InitializeGameController();
    void CreateGameSystems();
    void CreateStages(int stageCount);

    void ProcessInput();
    void ProcessGameInput();
    void ProcessActorsInput();

    void UpdateGame();
    void UpdateActors(float deltaTime);

    void GenerateOutput();

    void CreatePlayer2();
    void CheckGameControllerConnected();

private:
    GLFWwindow* mWindow = nullptr;
    SDL_GameController* mSdlController = nullptr;

    std::vector<Player*> mPlayers;
    std::vector<std::unique_ptr<Actor>> mActors;
    std::vector<Stage*> mStages;
    std::vector<std::unique_ptr<Stage>> mStagesUnique;

    std::unique_ptr<AudioSystem> mAudioSystem;
    std::unique_ptr<UIRenderer> mUIRenderer;
    std::unique_ptr<Renderer3D> mRenderer3D;
    std::unique_ptr<PhysicsSystem> mPhysicsSystem;
    std::unique_ptr<CameraSystem> mCameraSystem;
    std::unique_ptr<ActorLoadSystem> mActorLoadSystem;
    std::unique_ptr<MeshLoadSystem> mMeshLoadSystem;
    std::unique_ptr<MathUtils> mMathUtils;
    std::unique_ptr<SceneSystem> mSceneSystem;

    Stage* mCurrentStage = nullptr;

    int mCurrentStageNum = 0;
    float mHitStopTimer = -1.0f;

    double mLastTime = 0.0;

    bool mReloadKeyPressedPrev = false;
    bool mUIReloadKeyPressedPrev = false;
    bool mAPressedPrev = false;
    bool mPPressedPrev = false;
    bool mZLPressedPrev = false;
    bool mStartPressedPrev = false;
    bool mIsPlayer2Joined = false;

    std::string mCurrentStageYamlPath;
};