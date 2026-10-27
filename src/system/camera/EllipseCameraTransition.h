#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vector>

class Actor;
class CameraCollisionResolver;
class Planet;

struct EllipseCameraTransitionInput {
    const Planet* planet = nullptr;
    glm::vec3 playerPosition{0.0f};
    // 通常追従で補間したPlayer位置。注視点高さを加える前の、距離・ピッチの基準。
    glm::vec3 framingCenter{0.0f};
    glm::vec3 movementDirection{0.0f};
    glm::vec3 up{0, 1, 0};
    glm::vec3 playerUp{0, 1, 0};
    glm::vec3 cameraPosition{0.0f};
    glm::vec3 cameraBack{0, 0, 1};
    float movementSpeed = 0.0f;
    float distance = 8.0f;
    float pitch = -1.0f;
    float clearanceRadius = 0.3f;
    bool allowsStart = true;
};

// カメラ位置と、その位置の計算に使った注視中心・姿勢を一緒に引き継ぐ。
struct EllipseCameraTransitionPose {
    glm::vec3 position{0.0f};
    glm::vec3 center{0.0f};
    glm::vec3 up{0, 1, 0};
    glm::vec3 back{0, 0, 1};
};

// 楕円惑星の表裏を越えたとき、外周を通るカメラ位置を生成する。Planet・Actorは借用する。
// 通常追従や入力の保持は呼出し側で扱い、Sphereと平面には適用しない。
class EllipseCameraTransition {
public:
    // deltaTimeは秒。回り込み後に注視中心・上方向の遅れを解消し、通常構図へ戻して完了する。
    // 初回観測では現在の面を記録する。衝突補正は呼出し側へ委ねる。
    std::optional<EllipseCameraTransitionPose> Update(const EllipseCameraTransitionInput& input,
                                                      const CameraCollisionResolver& collision,
                                                      const std::vector<Actor*>& ignoredActors, float deltaTime);
    void Reset();
    bool IsActive() const { return mIsActive; }

    // 同じフレームのビュー再取得にも使える最新の姿勢。Update・Resetで更新される。
    const std::optional<EllipseCameraTransitionPose>& GetPose() const { return mPose; }

private:
    glm::vec3 CalculatePosition(float progress, float angle) const;
    std::vector<glm::vec3> CalculatePath(float angle) const;
    bool TryStart(const EllipseCameraTransitionInput& input, const CameraCollisionResolver& collision,
                  const std::vector<Actor*>& ignoredActors);
    bool UpdateDestination(const EllipseCameraTransitionInput& input, bool preservesRoute);
    int ResolveHemisphere(const EllipseCameraTransitionInput& input) const;

    const Planet* mPlanet = nullptr;
    glm::vec3 mCenter{0.0f};
    glm::vec3 mRadii{1.0f};
    glm::vec3 mFromDirection{0, 1, 0};
    glm::vec3 mAxis{1, 0, 0};
    glm::vec3 mStartUp{0, 1, 0};
    glm::vec3 mStartBack{0, 0, 1};
    EllipseCameraTransitionPose mDestination;
    std::optional<EllipseCameraTransitionPose> mPose;
    float mFromRadius = 1.0f;
    float mToRadius = 1.0f;
    float mAngle = 0.0f;
    float mClearanceRadius = 0.3f;
    float mElapsed = 0.0f;
    float mDuration = 0.7f;
    float mFramingElapsed = 0.0f;
    int mHemisphere = 0;
    bool mIsActive = false;
    bool mIsRestoringFraming = false;
};
