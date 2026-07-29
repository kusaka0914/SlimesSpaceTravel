#include "MathUtils.h"
#include "actor/Actor.h"
#include "actor/Planet.h"

#include <glm/gtc/matrix_transform.hpp>

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
    glm::vec3 upN = glm::normalize(actor->GetUpVec());
    glm::vec3 worldLeft = glm::cross(upN, glm::vec3(0, 0, 1));
    if (glm::length(worldLeft) < 0.01f) {
        worldLeft = glm::normalize(glm::cross(upN, glm::vec3(0, 1, 0)));
    } else
        worldLeft = glm::normalize(worldLeft);

    float actorYaw = actor->GetFacingYaw();
    glm::vec3 fwd = glm::normalize(glm::cross(worldLeft, upN) * std::cos(actorYaw) - std::sin(actorYaw) * worldLeft);
    glm::vec3 left = glm::normalize(glm::cross(upN, fwd));
    glm::vec3 right = -left;
    glm::mat4 orient = glm::mat4(1.0f);
    orient[0] = glm::vec4(-fwd, 0.0f);
    orient[1] = glm::vec4(upN, 0.0f);
    orient[2] = glm::vec4(right, 0.0f);
    orient[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    return orient;
}

glm::vec3 MathUtils::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);
    Planet* planet = actor->GetCurrentPlanet();

    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        const glm::vec3 toActor = actor->GetPos() - planet->GetPos();
        if (glm::length(toActor) > 1e-6f) {
            baseUp = glm::normalize(toActor);
        }
    }

    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);
    baseForward -= baseUp * glm::dot(baseForward, baseUp);

    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward -= baseUp * glm::dot(baseForward, baseUp);
    }

    baseForward = glm::normalize(baseForward);
    const glm::vec3 baseRight = glm::normalize(glm::cross(baseForward, baseUp));

    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix = glm::rotate(rotationMatrix, rotationRad.y, baseUp);
    rotationMatrix = glm::rotate(rotationMatrix, rotationRad.x, baseRight);
    rotationMatrix = glm::rotate(rotationMatrix, rotationRad.z, baseForward);

    const glm::vec3 upVec = glm::vec3(rotationMatrix * glm::vec4(baseUp, 0.0f));
    return glm::length(upVec) > 1e-6f ? glm::normalize(upVec) : baseUp;
}

void MathUtils::ApplyActorEditorRotation(Actor* actor) const
{
    if (!actor) {
        return;
    }

    const glm::vec3 rotation = actor->GetEditorRotation();
    actor->SetFacingYaw(rotation.y);
    actor->SetUpVec(CalculateActorUpVecFromEditorRotation(actor, rotation));
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
