#include "actor/MathUtils.h"

#include "actor/Actor.h"
#include "actor/Planet.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
glm::vec3 NormalizeOr(const glm::vec3& value, const glm::vec3& fallback)
{
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
        return fallback;
    }
    const float lengthSquared = glm::dot(value, value);
    if (lengthSquared < 1e-12f) {
        return fallback;
    }
    // 通常の計算順を保ち、二乗和が溢れる大きな座標だけ先に縮小する。
    if (std::isfinite(lengthSquared)) {
        return glm::normalize(value);
    }
    const float magnitude = std::max({std::abs(value.x), std::abs(value.y), std::abs(value.z)});
    return glm::normalize(value / magnitude);
}

glm::mat4 CreateBillboardBasis(const glm::mat4& inverseView, const glm::vec3& center, const glm::vec3& up)
{
    const glm::vec3 cameraFacing = NormalizeOr(glm::vec3(inverseView[2]), {0, 0, 1});
    const glm::vec3 forward = NormalizeOr(glm::vec3(inverseView[3]) - center, cameraFacing);
    glm::vec3 right = glm::cross(up, forward);
    if (glm::dot(right, right) < 1e-12f) {
        // 視線と上方向が平行な場合も、視線に直交する基準軸から平面を作る。
        const glm::vec3 reference = std::abs(forward.z) < 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
        right = glm::cross(reference, forward);
    }
    right = NormalizeOr(right, {1, 0, 0});

    glm::mat4 basis(1);
    basis[0] = glm::vec4(right, 0);
    basis[1] = glm::vec4(-glm::cross(forward, right), 0);
    basis[2] = glm::vec4(forward, 0);
    return basis;
}

glm::quat CreateSurfaceBaseOrientation(Actor* actor)
{
    if (!actor) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);
    Planet* planet = actor->GetCurrentPlanet();

    if (planet) {
        if (planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
            const glm::vec3 toActor = actor->GetPos() - planet->GetPos();
            if (glm::length(toActor) > 1e-6f) {
                baseUp = glm::normalize(toActor);
            }
        } else if (planet->GetPlanetShape() == Planet::PlanetShape::Ellipse) {
            baseUp = planet->CalculateEllipseVerticalDirection(actor->GetPos());
        }
    }

    // 基準の前方向を地表の接平面へ射影し、法線と平行になる場合だけ別の軸を使う。
    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);
    baseForward -= baseUp * glm::dot(baseForward, baseUp);
    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward -= baseUp * glm::dot(baseForward, baseUp);
    }
    baseForward = glm::normalize(baseForward);

    const glm::vec3 baseLeft = glm::normalize(glm::cross(baseUp, baseForward));

    glm::mat3 basis(1.0f);
    basis[0] = baseLeft;
    basis[1] = baseUp;
    basis[2] = baseForward;
    return glm::normalize(glm::quat_cast(basis));
}
} // namespace

float MathUtils::GetYawFromDirection(const glm::vec3& up, const glm::vec3& direction) const
{
    glm::vec3 baseLeft = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::normalize(glm::cross(up, glm::vec3(0.0f, 1.0f, 0.0f)));
    } else {
        baseLeft = glm::normalize(baseLeft);
    }
    const glm::vec3 baseForward = glm::cross(baseLeft, up);
    return std::atan2(-glm::dot(direction, baseLeft), glm::dot(direction, baseForward));
}

glm::mat4 MathUtils::CreateOrient(Actor* actor) const
{
    if (!actor) {
        return glm::mat4(1.0f);
    }

    const glm::mat4 semanticOrientation = glm::mat4_cast(actor->GetOrientation());

    // 描画時だけモデル軸と回転オフセットを合成し、Actorの論理姿勢は変更しない。
    glm::mat4 modelAxisCorrection(1.0f);
    modelAxisCorrection[0] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    modelAxisCorrection[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    modelAxisCorrection[2] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);

    return semanticOrientation * modelAxisCorrection * glm::mat4_cast(actor->GetRenderModelRotationOffset());
}

glm::quat MathUtils::CalculateActorOrientationFromEditorRotation(Actor* actor, const glm::vec3& rotationRadians) const
{
    const glm::quat baseOrientation = CreateSurfaceBaseOrientation(actor);
    const glm::quat localOrientation = glm::quat(rotationRadians);
    return glm::normalize(baseOrientation * localOrientation);
}

glm::vec3 MathUtils::CalculateActorEditorRotationFromOrientation(Actor* actor, const glm::quat& orientation) const
{
    if (glm::length(orientation) < 1e-6f) {
        return glm::vec3(0.0f);
    }

    const glm::quat baseOrientation = CreateSurfaceBaseOrientation(actor);

    // 地表基準の回転を取り除き、編集UIで扱うローカル回転へ戻す。
    const glm::quat localOrientation = glm::normalize(glm::inverse(baseOrientation) * glm::normalize(orientation));
    return glm::eulerAngles(localOrientation);
}

glm::vec3 MathUtils::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRadians) const
{
    const glm::quat orientation = CalculateActorOrientationFromEditorRotation(actor, rotationRadians);
    return glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));
}

void MathUtils::ApplyActorEditorRotation(Actor* actor) const
{
    if (!actor) {
        return;
    }

    actor->SetOrientation(CalculateActorOrientationFromEditorRotation(actor, actor->GetEditorRotation()));
}

glm::mat4 MathUtils::CreateBillBoard(const glm::mat4& viewMatrix, const Actor* actor, float upMargin, float rightMargin,
                                     float width, float height) const
{
    const glm::vec3 actorPosition = actor->GetPos();
    const glm::vec3 actorUpDirection = NormalizeOr(actor->GetUpVec(), {0, 1, 0});

    // 向きを決める注視点と描画位置を分け、表示マージンを変えてもカメラへの向きは保つ。
    const glm::vec3 facingCenter = actorPosition + actorUpDirection * 0.8f;
    glm::mat4 billboard = CreateBillboardBasis(glm::inverse(viewMatrix), facingCenter, actorUpDirection);
    const glm::vec3 right(billboard[0]);
    billboard[0] *= width;
    billboard[1] *= height;
    const glm::vec3 drawPosition = actorPosition + actorUpDirection * upMargin + right * rightMargin;
    billboard[3] = glm::vec4(drawPosition, 1.0f);

    return billboard;
}

glm::mat4 MathUtils::CreateBillBoard(const glm::mat4& viewMatrix, const glm::vec3& centerPosition,
                                     const glm::vec3& upDirection, float width, float height) const
{
    const glm::vec3 up = NormalizeOr(upDirection, {0, 1, 0});
    glm::mat4 billboard = CreateBillboardBasis(glm::inverse(viewMatrix), centerPosition, up);
    billboard[0] *= width;
    billboard[1] *= height;
    billboard[3] = glm::vec4(centerPosition, 1.0f);

    return billboard;
}
