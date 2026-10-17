#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

class Actor;
class CameraCollisionResolver;
class Game;

// Actorへの注目位置とカメラ位置を計算し、注目対象間の遷移状態を保持する。
class FocusCamera {
public:
    FocusCamera(Game* game, CameraCollisionResolver& collisionResolver);

    // 対象群を収める位置を選び、保持するカメラ姿勢を更新する。空の対象群では単位行列を返す。
    glm::mat4 GetFocusView(const std::vector<Actor*>& focusActors, const glm::vec3& preferredCameraPos);

    // Actorへ注目する姿勢を補間し、保持するカメラ姿勢も更新する。nullptrでは単位行列を返す。
    glm::mat4 GetCloseFocusView(Actor* focusActor, float cameraDistance, float cameraHeight, float targetHeight);
    void BeginTransition(const glm::vec3& cameraPos, const glm::vec3& targetPos, const glm::vec3& upVec);
    glm::mat4 GetTargetCameraView(Actor* targetActor);
    std::vector<glm::mat4> GetOpeningViews() const;
    const glm::vec3& GetCameraPos() const { return mCameraPos; }

private:
    Game* mGame;
    CameraCollisionResolver& mCollisionResolver;

    glm::vec3 mCameraUpVec{0.0f, 1.0f, 0.0f};
    glm::vec3 mCameraTargetPos{0.0f};
    glm::vec3 mCameraPos{0.0f};
};
