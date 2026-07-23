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

class CameraSystem {
public:
    using PlayerCameraState = ::PlayerCameraState;

    explicit CameraSystem(Game* game);

    void ProcessInput();
    void Update(float deltaTime);

    bool PlayCinematic(std::string_view sequenceId);
    void StopCinematic();

    bool IsCinematicPlaying() const { return mCinematicCamera.IsPlaying(); }
    bool HasCinematicFinished() const { return mCinematicCamera.HasFinished(); }

    float GetCinematicElapsedTime() const { return mCinematicCamera.GetElapsedTime(); }
    float GetCinematicDuration() const { return mCinematicCamera.GetDuration(); }

    bool SaveCinematicSequences() const { return mCinematicLibrary.Save(); }
    bool ReloadCinematicSequences();

    const PlayerCameraSettings& GetPlayerCameraSettings() const { return mPlayerCameraSettings; }
    void SetPlayerCameraSettings(PlayerCameraSettings settings);
    bool SavePlayerCameraSettings() const;
    bool ReloadPlayerCameraSettings();

    CinematicSequenceLibrary& GetCinematicLibrary() { return mCinematicLibrary; }
    const CinematicSequenceLibrary& GetCinematicLibrary() const { return mCinematicLibrary; }

    CameraPose GetDebugCameraPose() const { return mDebugCamera.GetPose(); }
    void SetDebugCameraPose(const CameraPose& pose) { mDebugCamera.SetPose(pose); }

    float GetFieldOfViewDegrees() const;

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    std::vector<glm::mat4> GetViews();
    glm::vec3 GetCameraPos() const;
    glm::vec3 GetPlayerCameraPos(int playerNum) const;

private:
    void UpdateCamera(float deltaTime);
    Enemy* FindBossEnemy(Planet* planet) const;

private:
    Game* mGame;

    bool mIsTargetFocus = false;
    float mCameraStickX = 0.0f;

    CameraCollisionResolver mCollisionResolver;
    DebugCamera mDebugCamera;
    FocusCamera mFocusCamera;
    PlayerCamera mPlayerCamera;
    PlayerCameraSettings mPlayerCameraSettings;
    PlayerCameraSettingsRepository mPlayerCameraSettingsRepository;

    CinematicSequenceLibrary mCinematicLibrary;
    CinematicCamera mCinematicCamera;
};
