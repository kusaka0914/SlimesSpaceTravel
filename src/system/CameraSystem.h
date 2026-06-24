#pragma once

#include <glm/glm.hpp>
#include <vector>

class Game;
class Actor;
class Player;
class Boat;
class SceneSystem;
class Planet;
class Key;

class CameraSystem {
public:
    struct PlayerCameraState {
        glm::vec3 cameraPos{0.0f};
        glm::vec3 targetPos{0.0f};
        glm::vec3 upVec{0.0f, 1.0f, 0.0f};
    };

    CameraSystem(Game* game);

    void ProcessInput();

    void Update(float deltaTime);

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    std::vector<glm::mat4> GetViews();
    glm::vec3 GetCameraPos() const { return mCameraPos; }

    const glm::vec3& GetPlayerCameraPos(int playerNum) const { return mPlayerCameraStates[playerNum].cameraPos; }

private:
    void UpdateCamera(float deltaTime);
    glm::vec3 ResolveCameraCollision(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos) const;

    std::vector<glm::mat4> GetOpeningViews() const;
    glm::mat4 GetPlayerView(Player* player, int playerIndex, float cameraDistance, bool isFixed = false);
    std::vector<glm::mat4> GetBoatFocusViews(const std::vector<Boat*>& boats) const;
    glm::mat4 GetDebugCameraView();
    glm::mat4 GetFocusView(Actor* focusActor) const;
    glm::mat4 GetTargetCameraView(Actor* targetActor);

    void ResizePlayerCameraState(std::size_t count);
    void UpdatePlayerCameraState(Player* player, int playerIndex, float deltaTime);

private:
    bool mIsTargetFocus;

    float mCameraPitch;
    float mCameraStickX;
    float mMoveForward;
    float mMoveRight;
    float mMoveUp;
    float mDebugCameraYaw;
    float mDebugCameraPitch;
    float mDebugYawInput;
    float mDebugPitchInput;

    glm::vec3 mCameraUpVec;
    glm::vec3 mCameraTargetPos;
    glm::vec3 mCameraPos;

    std::vector<PlayerCameraState> mPlayerCameraStates;

    Game* mGame;
};