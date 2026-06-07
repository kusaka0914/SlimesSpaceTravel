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
    CameraSystem(Game* game);

    void ProcessInput();

    void Update(float deltaTime);

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    std::vector<glm::mat4> GetViews();
    glm::vec3 GetCameraPos() const { return mCameraPos; }

private:
    void UpdateCamera(float deltaTime);

    std::vector<glm::mat4> GetOpeningViews() const;
    std::vector<glm::mat4> GetBoatFocusViews(std::vector<Boat*> boats) const;
    glm::mat4 GetPlayerView(Player* player, float cameraDistance, bool isFixed = false);
    glm::mat4 GetDebugCameraView();
    glm::mat4 GetFocusView(Actor* focusActor) const;
    glm::mat4 GetTargetCameraView(Actor* targetActor);

private:
    bool mIsTargetFocus;

    float mCameraYaw;
    float mCameraPitch;
    float mCameraStickY;
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

    Game* mGame;
};