#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

class Actor;
class Boat;
class Game;

class FocusCamera {
public:
    explicit FocusCamera(Game* game);

    glm::mat4 GetFocusView(Actor* focusActor) const;
    glm::mat4 GetCloseFocusView(Actor* focusActor, float cameraDistance, float cameraHeight,
                                float targetHeight);
    void BeginTransition(const glm::vec3& cameraPos, const glm::vec3& targetPos, const glm::vec3& upVec);
    glm::mat4 GetTargetCameraView(Actor* targetActor);
    std::vector<glm::mat4> GetOpeningViews() const;
    std::vector<glm::mat4> GetBoatFocusViews(const std::vector<Boat*>& boats) const;

    const glm::vec3& GetCameraPos() const { return mCameraPos; }

private:
    Game* mGame;

    glm::vec3 mCameraUpVec{0.0f, 1.0f, 0.0f};
    glm::vec3 mCameraTargetPos{0.0f};
    glm::vec3 mCameraPos{0.0f};
};
