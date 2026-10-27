#include "system/camera/EllipseCameraTransition.h"

#include "actor/Planet.h"
#include "system/camera/CameraCollisionResolver.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;
constexpr float framingRestoreSeconds = 0.35f;

glm::vec3 TangentDirection(const glm::vec3& direction, const glm::vec3& up, const glm::vec3& fallback)
{
    const auto tangent = direction - up * glm::dot(direction, up);
    return glm::dot(tangent, tangent) > directionEpsilonSquared ? glm::normalize(tangent) : fallback;
}
} // namespace

void EllipseCameraTransition::Reset()
{
    *this = {};
}

std::optional<EllipseCameraTransitionPose> EllipseCameraTransition::Update(const EllipseCameraTransitionInput& input,
                                                                           const CameraCollisionResolver& collision,
                                                                           const std::vector<Actor*>& ignoredActors,
                                                                           float deltaTime)
{
    mPose.reset();
    if (!input.planet || input.planet->GetPlanetShape() != Planet::PlanetShape::Ellipse) {
        Reset();
        return std::nullopt;
    }
    const auto radii = glm::abs(input.planet->GetScale());
    if (std::min({radii.x, radii.y, radii.z}) < 0.001f) {
        Reset();
        return std::nullopt;
    }
    if (mPlanet != input.planet || glm::distance(mRadii, radii) > 0.001f ||
        glm::distance(mCenter, input.planet->GetPos()) > 0.001f) {
        Reset();
        mPlanet = input.planet;
        mRadii = radii;
        mCenter = input.planet->GetPos();
    }
    const float elapsed = std::max(0.0f, deltaTime);
    const int hemisphere = ResolveHemisphere(input);
    const bool crossedBoundary = mHemisphere != 0 && mHemisphere != hemisphere;
    mHemisphere = hemisphere;
    if (!mIsActive) {
        if (!crossedBoundary || elapsed <= 0.0f || !input.allowsStart || !TryStart(input, collision, ignoredActors)) {
            return std::nullopt;
        }
    } else if (mIsRestoringFraming) {
        mFramingElapsed += elapsed;
        const float progress = std::min(1.0f, mFramingElapsed / framingRestoreSeconds);
        const float blend = progress * progress * (3.0f - 2.0f * progress);
        auto framing = input;
        const auto up = glm::dot(input.up, input.up) > directionEpsilonSquared ? glm::normalize(input.up) : mStartUp;
        const auto playerUp =
            glm::dot(input.playerUp, input.playerUp) > directionEpsilonSquared ? glm::normalize(input.playerUp) : up;
        framing.up = glm::slerp(glm::quat(1, 0, 0, 0), glm::quat(up, playerUp), blend) * up;
        framing.framingCenter = glm::mix(input.framingCenter, input.playerPosition, blend);
        if (!UpdateDestination(framing, true)) {
            Reset();
            return std::nullopt;
        }
        mPose = mDestination;
        mIsActive = progress < 1.0f;
        return mPose;
    } else {
        if (!UpdateDestination(input, true)) {
            Reset();
            return std::nullopt;
        }
        mElapsed += elapsed;
    }
    const float progress = std::min(1.0f, mElapsed / mDuration);
    mPose = mDestination;
    mPose->position = CalculatePosition(progress, mAngle);
    if (progress >= 1.0f) {
        // 位置だけ先に固定すると、残った注視点・上方向の補間が見下ろし角を変えてしまう。
        mIsRestoringFraming = true;
        mFramingElapsed = 0.0f;
    }
    return mPose;
}

int EllipseCameraTransition::ResolveHemisphere(const EllipseCameraTransitionInput& input) const
{
    int shortestAxis = 0;
    for (int axis = 1; axis < 3; ++axis) {
        if (mRadii[axis] < mRadii[shortestAxis]) {
            shortestAxis = axis;
        }
    }
    const float offset = input.playerPosition[shortestAxis] - mCenter[shortestAxis];
    // 表裏の境界直上で往復する浮動小数点誤差・接地補正だけでは、切り替えを繰り返さない。
    const float boundaryMargin = std::max(0.001f, mRadii[shortestAxis] * 0.01f);
    if (mHemisphere != 0 && std::abs(offset) < boundaryMargin) {
        return mHemisphere;
    }
    return input.planet->ResolveEllipseSurfaceHemisphere(input.playerPosition) == Planet::EllipseSurfaceFace::Front
               ? 1
               : -1;
}

bool EllipseCameraTransition::TryStart(const EllipseCameraTransitionInput& input,
                                       const CameraCollisionResolver& collision,
                                       const std::vector<Actor*>& ignoredActors)
{
    if (glm::dot(input.movementDirection, input.movementDirection) <= directionEpsilonSquared ||
        input.movementSpeed <= 0.001f) {
        return false;
    }
    const auto surface = input.planet->CalculateEllipseSurfaceProjection(input.playerPosition);
    const auto direction = TangentDirection(input.movementDirection, surface.outwardNormal, glm::vec3(0));
    if (glm::dot(direction, direction) <= directionEpsilonSquared) {
        return false;
    }
    const auto surfaceDirection = glm::normalize((surface.position - mCenter) / mRadii);
    const auto scaledTangent = direction / mRadii;
    const auto tangent = TangentDirection(scaledTangent, surfaceDirection, glm::vec3(0));
    if (glm::dot(tangent, tangent) <= directionEpsilonSquared) {
        return false;
    }
    mStartUp =
        glm::dot(input.up, input.up) > directionEpsilonSquared ? glm::normalize(input.up) : surface.outwardNormal;
    const auto fallbackBack = -TangentDirection(direction, mStartUp, tangent);
    mStartBack = TangentDirection(input.cameraBack, mStartUp, fallbackBack);
    const auto from = (input.cameraPosition - mCenter) / mRadii;
    mFromRadius = glm::length(from);
    if (mFromRadius <= 0.001f) {
        return false;
    }
    mFromDirection = from / mFromRadius;
    mAxis = glm::normalize(glm::cross(surfaceDirection, tangent));
    if (!UpdateDestination(input, false)) {
        return false;
    }
    mClearanceRadius = std::max(0.05f, input.clearanceRadius);
    auto path = CalculatePath(mAngle);
    if (mAngle > 0.01f) {
        const float obstruction = collision.MeasureCameraPathObstruction(path, mClearanceRadius, ignoredActors);
        if (obstruction > 0.0f) {
            const float oppositeAngle = mAngle - glm::two_pi<float>();
            auto opposite = CalculatePath(oppositeAngle);
            if (collision.MeasureCameraPathObstruction(opposite, mClearanceRadius, ignoredActors) + 0.01f <
                obstruction) {
                mAngle = oppositeAngle;
                path = std::move(opposite);
            }
        }
    }
    float distance = 0.0f;
    for (std::size_t index = 1; index < path.size(); ++index) {
        distance += glm::distance(path[index - 1], path[index]);
    }
    mDuration = std::clamp(distance / 25.0f, 0.5f, 1.2f);
    mElapsed = 0.0f;
    mFramingElapsed = 0.0f;
    mIsRestoringFraming = false;
    mIsActive = true;
    return true;
}

bool EllipseCameraTransition::UpdateDestination(const EllipseCameraTransitionInput& input, bool preservesRoute)
{
    const auto up = glm::dot(input.up, input.up) > directionEpsilonSquared ? glm::normalize(input.up) : mStartUp;
    const auto back = glm::quat(mStartUp, up) * mStartBack;
    // 停止・方向変更後も、現在の通常構図へ戻せるよう到達先を毎回更新する。
    mDestination.center = input.framingCenter;
    mDestination.up = up;
    mDestination.back = back;
    mDestination.position =
        input.framingCenter + (back * std::cos(input.pitch) - up * std::sin(input.pitch)) * input.distance;
    const auto to = (mDestination.position - mCenter) / mRadii;
    mToRadius = glm::length(to);
    if (mToRadius <= 0.001f) {
        return false;
    }
    const auto toDirection = to / mToRadius;
    const auto cross = glm::cross(mFromDirection, toDirection);
    float angle = std::acos(std::clamp(glm::dot(mFromDirection, toDirection), -1.0f, 1.0f));
    if (glm::dot(cross, cross) > directionEpsilonSquared) {
        auto axis = glm::normalize(cross);
        if (preservesRoute && glm::dot(axis, mAxis) < 0.0f) {
            axis = -axis;
            angle = -angle;
        }
        mAxis = axis;
    }
    if (preservesRoute) {
        // 動く到達先が正反対・同方向を跨いでも、選択済みの回転経路を急に反転させない。
        angle += glm::two_pi<float>() * std::round((mAngle - angle) / glm::two_pi<float>());
    }
    mAngle = angle;
    return true;
}

glm::vec3 EllipseCameraTransition::CalculatePosition(float progress, float angle) const
{
    if (progress >= 1.0f) {
        // 通常構図を基準位置として返す。終点で必要な衝突補正は呼出し側の描画位置にだけ適用する。
        return mDestination.position;
    }
    const float blend = progress * progress * (3.0f - 2.0f * progress);
    const auto direction = glm::angleAxis(angle * blend, mAxis) * mFromDirection;
    float radius = glm::mix(mFromRadius, mToRadius, blend);
    if (progress > 0.0f) {
        // 表面の接平面より球半径分だけ外へ出し、楕円体内部を横切る直線補間を避ける。
        radius = std::max(radius, 1.0f + mClearanceRadius * glm::length(direction / mRadii));
    }
    return mCenter + mRadii * direction * radius;
}

std::vector<glm::vec3> EllipseCameraTransition::CalculatePath(float angle) const
{
    constexpr int segments = 48;
    std::vector<glm::vec3> path;
    path.reserve(segments + 1);
    for (int index = 0; index <= segments; ++index) {
        path.push_back(CalculatePosition(static_cast<float>(index) / segments, angle));
    }
    return path;
}
