#pragma once

#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CinematicCamera.h"
#include "system/camera/CinematicSequenceLibrary.h"
#include "system/camera/DebugCamera.h"
#include "system/camera/FocusCamera.h"
#include "system/camera/PlayerCamera.h"
#include "system/camera/PlayerCameraSettings.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

class Game;
class Actor;
class Player;
class Boat;
class Planet;
class Enemy;
class Star;

class CameraSystem {
public:
    using PlayerCameraState = ::PlayerCameraState;

    explicit CameraSystem(Game* game);

    void ProcessInput();
    void Update(float deltaTime);

    bool PlayCinematic(
        std::string_view sequenceId,
        bool shouldHoldFinalPose = false);
    void StopCinematic();

    bool IsCinematicPlaying() const { return mCinematicCamera.IsActive(); }
    bool HasCinematicFinished() const { return mCinematicCamera.HasFinished(); }

    float GetCinematicElapsedTime() const { return mCinematicCamera.GetElapsedTime(); }
    float GetCinematicDuration() const { return mCinematicCamera.GetDuration(); }

    bool SaveCinematicSequences() const { return mCinematicLibrary.Save(); }
    bool ReloadCinematicSequences();

    const PlayerCameraSettings& GetPlayerCameraSettings() const { return mPlayerCameraSettings; }
    void SetPlayerCameraSettings(PlayerCameraSettings settings);
    bool SavePlayerCameraSettings() const;
    bool ReloadPlayerCameraSettings();
    void ResetForStageChange();
    void SnapBehindControlledPlayer();

    bool GetTalkCameraPreviewEnabled() const { return mTalkCameraPreviewEnabled; }
    void SetTalkCameraPreviewEnabled(bool enabled) { mTalkCameraPreviewEnabled = enabled; }
    bool GetBoatRideCameraPreviewEnabled() const
    {
        return mBoatRideCameraPreviewEnabled;
    }
    void SetBoatRideCameraPreviewEnabled(bool enabled)
    {
        mBoatRideCameraPreviewEnabled = enabled;
    }

    CinematicSequenceLibrary& GetCinematicLibrary() { return mCinematicLibrary; }
    const CinematicSequenceLibrary& GetCinematicLibrary() const { return mCinematicLibrary; }

    CameraPose GetDebugCameraPose() const { return mDebugCamera.GetPose(); }
    void SetDebugCameraPose(const CameraPose& pose) { mDebugCamera.SetPose(pose); }

    float GetFieldOfViewDegrees() const;

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    bool AllowsPlayerInput() const;
    void SnapToControlledPlayer(int fromPlayerIndex, int toPlayerIndex);
    void StartBossDefeatSequence(Enemy* boss, Star* star);
    bool PreviewBossDefeatSequence();
    void StopBossDefeatSequence();
    bool IsBossDefeatSequencePlaying() const { return mBossDefeatSequenceTimer >= 0.0f; }
    std::vector<glm::mat4> GetViews();
    glm::vec3 GetCameraPos() const;
    glm::vec3 GetPlayerCameraPos(int playerNum) const;

private:
    void UpdateCamera(float deltaTime);
    void UpdateBossDefeatSequence(float deltaTime);
    void UpdateTalkCameraTransition(float deltaTime);
    void UpdateTalkCameraAim();
    void UpdateTalkPageFocus(float deltaTime);
    void UpdatePlayerPitchOffsets(float deltaTime);
    int GetPrimaryPlayerIndex() const;
    float GetEasedTalkCameraBlend() const;
    float GetEasedTalkPageFocusBlend() const;
    glm::mat4 GetPlayerCameraView(Player* player, int playerIndex);
    glm::mat4 GetTalkPageFocusView(Player* player, int playerIndex);
    Actor* ResolveTalkPageFocusActor() const;
    Boat* FindFocusingBoat() const;
    Boat* FindMovingBoat() const;
    Boat* ResolveBoatRideCameraTarget() const;
    Enemy* FindBossEnemy(Planet* planet) const;

private:
    Game* mGame;

    bool mIsTargetFocus = false;
    bool mTalkCameraPreviewEnabled = false;
    bool mBoatRideCameraPreviewEnabled = false;
    bool mAlignCameraPressedPrev = false;
    bool mSecondControllerAlignCameraPressedPrev = false;
    bool mBossDefeatSequenceIsPreview = false;
    bool mBossDefeatSEPlayed = false;
    float mBossDefeatSequenceTimer = -1.0f;
    float mCameraStickX = 0.0f;
    float mCameraStickY = 0.0f;
    float mSecondControllerStickX = 0.0f;
    float mSecondControllerStickY = 0.0f;
    float mKeyboardPitchInput = 0.0f;
    float mTalkCameraBlend = 0.0f;
    std::vector<float> mPlayerPitchOffsetsDegrees;
    Player* mTalkCameraPlayer = nullptr;
    bool mHasTalkCameraTarget = false;
    glm::vec3 mTalkCameraTargetPos{0.0f};
    Actor* mTalkPageFocusActor = nullptr;
    float mTalkPageFocusBlend = 0.0f;
    bool mHasTalkPageFocusPose = false;
    glm::vec3 mTalkPageFocusCameraPos{0.0f};
    glm::vec3 mTalkPageFocusTargetPos{0.0f};
    glm::vec3 mTalkPageFocusUpVec{0.0f, 1.0f, 0.0f};
    glm::vec3 mRenderedTalkPageCameraPos{0.0f};
    Enemy* mDefeatedBoss = nullptr;
    Star* mBossDefeatStar = nullptr;
    Boat* mBoatRideCameraTarget = nullptr;

    CameraCollisionResolver mCollisionResolver;
    DebugCamera mDebugCamera;
    FocusCamera mFocusCamera;
    PlayerCamera mPlayerCamera;
    PlayerCameraSettings mPlayerCameraSettings;
    PlayerCameraSettingsRepository mPlayerCameraSettingsRepository;

    CinematicSequenceLibrary mCinematicLibrary;
    CinematicCamera mCinematicCamera;
};
