#include "MathUtils.h"
#include "actor/Actor.h"
#include "actor/Planet.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {
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
            baseUp =
                planet->CalculateEllipseVerticalDirection(actor->GetPos());
        }
    }

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

float MathUtils::GetYawFromDirection(const glm::vec3& up, const glm::vec3& dir) const
{
    glm::vec3 baseLeft = glm::cross(up, glm::vec3(0, 0, 1));
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::normalize(glm::cross(up, glm::vec3(0, 1, 0)));
    } else
        baseLeft = glm::normalize(baseLeft);
    glm::vec3 baseForward = glm::cross(baseLeft, up);
    return std::atan2(-glm::dot(dir, baseLeft), glm::dot(dir, baseForward));
}

glm::mat4 MathUtils::CreateOrient(Actor* actor) const
{
    if (!actor) {
        return glm::mat4(1.0f);
    }

    const glm::mat4 semanticOrientation = glm::mat4_cast(actor->GetOrientation());

    glm::mat4 modelAxisCorrection(1.0f);
    modelAxisCorrection[0] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    modelAxisCorrection[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    modelAxisCorrection[2] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);

    return semanticOrientation * modelAxisCorrection;
}

glm::quat MathUtils::CalculateActorOrientationFromEditorRotation(Actor* actor,
                                                                 const glm::vec3& rotationRad) const
{
    const glm::quat baseOrientation = CreateSurfaceBaseOrientation(actor);
    const glm::quat localOrientation = glm::quat(rotationRad);
    return glm::normalize(baseOrientation * localOrientation);
}

glm::vec3 MathUtils::CalculateActorEditorRotationFromOrientation(Actor* actor, const glm::quat& orientation) const
{
    if (glm::length(orientation) < 1e-6f) {
        return glm::vec3(0.0f);
    }

    const glm::quat baseOrientation = CreateSurfaceBaseOrientation(actor);
    const glm::quat localOrientation = glm::normalize(glm::inverse(baseOrientation) * glm::normalize(orientation));
    return glm::eulerAngles(localOrientation);
}

glm::vec3 MathUtils::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    const glm::quat orientation = CalculateActorOrientationFromEditorRotation(actor, rotationRad);
    return glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));
}

void MathUtils::ApplyActorEditorRotation(Actor* actor) const
{
    if (!actor) {
        return;
    }

    actor->SetOrientation(CalculateActorOrientationFromEditorRotation(actor, actor->GetEditorRotation()));
}

glm::mat4 MathUtils::CreateBillBoard(const glm::mat4& viewMat, const Actor* actor, float upMargin, float rightMargin,
                                     float width, float height) const
{
    glm::vec3 cameraPos(glm::inverse(viewMat)[3]);
    glm::vec3 actorPos = actor->GetPos();
    glm::vec3 actorUpVec = actor->GetUpVec();
    glm::vec3 quadCenter = actorPos + actorUpVec * 0.8f;
    glm::vec3 forward = glm::normalize(cameraPos - quadCenter);
    glm::vec3 right = glm::normalize(glm::cross(actorUpVec, forward));
    if (glm::length(right) < 0.01f)
        right = glm::normalize(glm::cross(actorUpVec, glm::vec3(0, 0, 1)));
    glm::vec3 upQuad = glm::cross(forward, right);

    glm::mat4 billboard(1.0f);
    billboard[0] = glm::vec4(right * width, 0.0f);
    billboard[1] = glm::vec4(-upQuad * height, 0.0f);
    billboard[2] = glm::vec4(forward, 0.0f);
    glm::vec3 drawPos = actorPos + actorUpVec * upMargin + right * rightMargin;
    billboard[3] = glm::vec4(drawPos, 1.0f);

    return billboard;
}

glm::mat4 MathUtils::CreateBillBoard(const glm::mat4& viewMat, const glm::vec3& centerPos, const glm::vec3& upVec,
                                     float width, float height) const
{
    glm::vec3 cameraPos(glm::inverse(viewMat)[3]);

    glm::vec3 up = upVec;
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = glm::normalize(cameraPos - centerPos);
    glm::vec3 right = glm::normalize(glm::cross(up, forward));

    if (glm::length(right) < 0.01f) {
        right = glm::normalize(glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f)));
    }
    if (glm::length(right) < 0.01f) {
        right = glm::normalize(glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    glm::vec3 upQuad = glm::cross(forward, right);

    glm::mat4 billboard(1.0f);
    billboard[0] = glm::vec4(right * width, 0.0f);
    billboard[1] = glm::vec4(-upQuad * height, 0.0f);
    billboard[2] = glm::vec4(forward, 0.0f);
    billboard[3] = glm::vec4(centerPos, 1.0f);

    return billboard;
}
