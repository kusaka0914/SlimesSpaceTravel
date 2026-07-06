#pragma once

#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/DebugCamera.h"
#include "system/camera/FocusCamera.h"
#include "system/camera/PlayerCamera.h"

#include <glm/glm.hpp>
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
    float mCameraPitch = -1.0f;
    float mCameraStickX = 0.0f;

    CameraCollisionResolver mCollisionResolver;
    DebugCamera mDebugCamera;
    FocusCamera mFocusCamera;
    PlayerCamera mPlayerCamera;
};
