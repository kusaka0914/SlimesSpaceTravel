#include "system/camera/FocusCamera.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "component/PlatformMovementComponent.h"
#include "system/SceneSystem.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

namespace {
glm::vec3 NormalizeOrFallback(const glm::vec3& direction, const glm::vec3& fallback)
{
    if (glm::dot(direction, direction) < 0.000001f) {
        return fallback;
    }
    return glm::normalize(direction);
}

glm::vec3 CalculateTangentDirection(
    const glm::vec3& direction,
    const glm::vec3& up)
{
    const glm::vec3 tangent = direction - up * glm::dot(direction, up);
    if (glm::dot(tangent, tangent) >= 0.000001f) {
        return glm::normalize(tangent);
    }

    glm::vec3 fallback = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
    if (glm::dot(fallback, fallback) < 0.000001f) {
        fallback = glm::cross(up, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    return glm::normalize(fallback);
}

float CalculateFocusRadius(const Actor& actor)
{
    const glm::vec3 scale = glm::abs(actor.GetScale());
    const float largestScale = std::max({scale.x, scale.y, scale.z, 1.0f});
    float focusRadius = std::max(actor.GetRadius() * largestScale, 1.0f);

    const LoadedModel* model = actor.GetLoadedModel();
    if (model && model->hasBounds) {
        const glm::vec3 scaledSize =
            glm::abs((model->boundsMaximum - model->boundsMinimum) * scale);
        focusRadius = std::max(focusRadius, glm::length(scaledSize) * 0.5f);
    }

    return glm::clamp(focusRadius, 1.0f, 24.0f);
}

struct FocusBounds {
    glm::vec3 center{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float radius = 1.0f;
};

FocusBounds CalculateFocusBounds(const std::vector<Actor*>& focusActors)
{
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(std::numeric_limits<float>::lowest());
    glm::vec3 combinedUp(0.0f);

    for (const Actor* actor : focusActors) {
        if (!actor) {
            continue;
        }

        const float actorRadius = CalculateFocusRadius(*actor);
        const glm::vec3 actorUp = NormalizeOrFallback(
            actor->GetUpVec(),
            glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 actorCenter =
            actor->GetPos() +
            actorUp * glm::clamp(actorRadius * 0.15f, 0.0f, 2.5f);
        const glm::vec3 radiusVector(actorRadius);
        minimum = glm::min(minimum, actorCenter - radiusVector);
        maximum = glm::max(maximum, actorCenter + radiusVector);

        const Platform* platform = dynamic_cast<const Platform*>(actor);
        const PlatformMovementComponent* movement =
            platform ? platform->GetMovementComponent() : nullptr;
        const Planet* planet = platform ? platform->GetCurrentPlanet() : nullptr;
        if (movement && planet) {
            const float centerHeight =
                glm::clamp(actorRadius * 0.15f, 0.0f, 2.5f);
            const glm::vec3 startCenter =
                planet->GetPos() + movement->GetBaseLocalPos() +
                actorUp * centerHeight;
            const glm::vec3 destinationCenter =
                planet->GetPos() + movement->GetDestinationLocalPos() +
                actorUp * centerHeight;
            minimum = glm::min(
                minimum,
                glm::min(
                    startCenter - radiusVector,
                    destinationCenter - radiusVector));
            maximum = glm::max(
                maximum,
                glm::max(
                    startCenter + radiusVector,
                    destinationCenter + radiusVector));
        }

        combinedUp += actorUp;
    }

    FocusBounds bounds;
    bounds.center = (minimum + maximum) * 0.5f;
    bounds.up = NormalizeOrFallback(combinedUp, glm::vec3(0.0f, 1.0f, 0.0f));
    bounds.radius = glm::clamp(glm::length(maximum - minimum) * 0.5f, 1.0f, 30.0f);
    return bounds;
}

glm::vec3 RotateAroundUp(
    const glm::vec3& direction,
    const glm::vec3& up,
    float angleRadians)
{
    return glm::normalize(
        direction * std::cos(angleRadians) +
        glm::cross(up, direction) * std::sin(angleRadians));
}

std::array<glm::vec3, 5> CreateVisibilitySamples(
    const FocusBounds& focusBounds,
    const glm::vec3& cameraBackDirection)
{
    glm::vec3 screenRight = glm::cross(
        focusBounds.up,
        cameraBackDirection);
    screenRight = NormalizeOrFallback(
        screenRight,
        CalculateTangentDirection(
            glm::vec3(1.0f, 0.0f, 0.0f),
            focusBounds.up));

    const float verticalOffset = focusBounds.radius * 0.45f;
    const float horizontalOffset = focusBounds.radius * 0.45f;
    return {
        focusBounds.center,
        focusBounds.center + focusBounds.up * verticalOffset,
        focusBounds.center - focusBounds.up * verticalOffset,
        focusBounds.center + screenRight * horizontalOffset,
        focusBounds.center - screenRight * horizontalOffset};
}

float CalculateVisibleSampleRatio(
    const CameraCollisionResolver& collisionResolver,
    const glm::vec3& cameraPos,
    const std::array<glm::vec3, 5>& visibilitySamples,
    const std::vector<Actor*>& ignoredActors)
{
    int visibleSampleCount = 0;
    for (const glm::vec3& samplePos : visibilitySamples) {
        if (collisionResolver.HasClearLineOfSight(
                cameraPos,
                samplePos,
                ignoredActors)) {
            ++visibleSampleCount;
        }
    }

    return static_cast<float>(visibleSampleCount) /
        static_cast<float>(visibilitySamples.size());
}

struct FocusCameraCandidateProfile {
    float distanceMultiplier = 1.0f;
    float heightMultiplier = 1.0f;
    float preferencePenalty = 0.0f;
};
}

FocusCamera::FocusCamera(Game* game, CameraCollisionResolver& collisionResolver)
    : mGame(game),
      mCollisionResolver(collisionResolver)
{
}

glm::mat4 FocusCamera::GetFocusView(
    const std::vector<Actor*>& focusActors,
    const glm::vec3& preferredCameraPos)
{
    if (focusActors.empty() || !focusActors.front()) {
        return glm::mat4(1.0f);
    }

    const FocusBounds focusBounds = CalculateFocusBounds(focusActors);
    const glm::vec3 up = focusBounds.up;
    const float focusRadius = focusBounds.radius;
    const glm::vec3 targetPos = focusBounds.center;

    glm::vec3 preferredBack = preferredCameraPos - targetPos;
    if (glm::dot(preferredBack, preferredBack) < 0.000001f &&
        mGame && mGame->GetMainPlayer()) {
        preferredBack = mGame->GetMainPlayer()->GetPos() - targetPos;
    }
    if (glm::dot(preferredBack, preferredBack) < 0.000001f) {
        preferredBack = -focusActors.front()->GetForwardVec();
    }
    preferredBack = CalculateTangentDirection(preferredBack, up);

    const float cameraDistance = glm::clamp(6.0f + focusRadius * 2.5f, 12.0f, 60.0f);
    const float cameraHeight = glm::clamp(2.5f + focusRadius * 0.45f, 3.0f, 13.0f);
    constexpr std::array<float, 8> candidateAnglesRadians = {
        0.0f,
        0.785398163f,
        -0.785398163f,
        1.570796327f,
        -1.570796327f,
        2.35619449f,
        -2.35619449f,
        3.141592654f};
    constexpr std::array<FocusCameraCandidateProfile, 2>
        candidateProfiles = {{
            {1.0f, 1.0f, 0.0f},
            {1.25f, 1.8f, 0.08f},
        }};
    constexpr float cameraClearanceRadius = 0.65f;
    const std::vector<Actor*> noIgnoredClearanceActors;

    glm::vec3 selectedCameraPos = preferredCameraPos;
    float bestScore = -std::numeric_limits<float>::infinity();
    for (const FocusCameraCandidateProfile& profile : candidateProfiles) {
        for (const float candidateAngleRadians : candidateAnglesRadians) {
            const glm::vec3 candidateBack = RotateAroundUp(
                preferredBack,
                up,
                candidateAngleRadians);
            const glm::vec3 desiredCameraPos =
                targetPos +
                candidateBack * cameraDistance * profile.distanceMultiplier +
                up * cameraHeight * profile.heightMultiplier;
            const glm::vec3 resolvedCameraPos = mCollisionResolver.Resolve(
                targetPos,
                desiredCameraPos,
                focusActors);
            if (!mCollisionResolver.HasCameraClearance(
                    resolvedCameraPos,
                    cameraClearanceRadius,
                    noIgnoredClearanceActors)) {
                continue;
            }

            const std::array<glm::vec3, 5> visibilitySamples =
                CreateVisibilitySamples(focusBounds, candidateBack);
            const float visibleSampleRatio =
                CalculateVisibleSampleRatio(
                    mCollisionResolver,
                    resolvedCameraPos,
                    visibilitySamples,
                    focusActors);
            const float desiredDistance =
                glm::length(desiredCameraPos - targetPos);
            const float resolvedDistance =
                glm::length(resolvedCameraPos - targetPos);
            const float resolvedDistanceRatio =
                desiredDistance > 0.001f
                    ? glm::clamp(
                          resolvedDistance / desiredDistance,
                          0.0f,
                          1.0f)
                    : 0.0f;
            const float directionChangePenalty =
                std::abs(candidateAngleRadians) * 0.04f;
            const float score =
                visibleSampleRatio * 2.0f +
                resolvedDistanceRatio -
                directionChangePenalty -
                profile.preferencePenalty;
            if (score > bestScore) {
                bestScore = score;
                selectedCameraPos = resolvedCameraPos;
            }
        }
    }

    mCameraPos = selectedCameraPos;
    mCameraTargetPos = targetPos;
    mCameraUpVec = up;
    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}
glm::mat4 FocusCamera::GetCloseFocusView(Actor* focusActor, float cameraDistance, float cameraHeight,
                                         float targetHeight)
{
    if (!focusActor) {
        return glm::mat4(1.0f);
    }

    glm::vec3 up = focusActor->GetUpVec();
    if (glm::length(up) < 0.001f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up = glm::normalize(up);
    }

    glm::vec3 back = mGame && mGame->GetMainPlayer()
                         ? mGame->GetMainPlayer()->GetPos() - focusActor->GetPos()
                         : glm::vec3(0.0f, 0.0f, 1.0f);
    back -= up * glm::dot(back, up);
    if (glm::length(back) < 0.001f) {
        back = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (glm::length(back) < 0.001f) {
        back = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    back = glm::normalize(back);

    const glm::vec3 desiredTargetPos = focusActor->GetPos() + up * targetHeight;
    const glm::vec3 desiredCameraPos =
        desiredTargetPos + back * cameraDistance + up * cameraHeight;

    constexpr float transitionBlend = 0.12f;
    mCameraPos = glm::mix(mCameraPos, desiredCameraPos, transitionBlend);
    mCameraTargetPos = glm::mix(mCameraTargetPos, desiredTargetPos, transitionBlend);
    mCameraUpVec = glm::normalize(glm::mix(mCameraUpVec, up, transitionBlend));

    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}

void FocusCamera::BeginTransition(const glm::vec3& cameraPos, const glm::vec3& targetPos,
                                  const glm::vec3& upVec)
{
    mCameraPos = cameraPos;
    mCameraTargetPos = targetPos;
    mCameraUpVec = glm::length(upVec) < 0.001f
                       ? glm::vec3(0.0f, 1.0f, 0.0f)
                       : glm::normalize(upVec);
}

glm::mat4 FocusCamera::GetTargetCameraView(Actor* targetActor)
{
    if (!mGame || !targetActor) {
        return glm::mat4(1.0f);
    }

    Player* player = mGame->GetMainPlayer();
    if (!player) {
        return glm::mat4(1.0f);
    }

    const glm::vec3 playerPos = player->GetPos();
    const glm::vec3 targetPos = targetActor->GetPos();

    constexpr float cameraLerp = 0.12f;

    glm::vec3 up = glm::normalize(player->GetUpVec());

    const glm::vec3 center = glm::mix(playerPos, targetPos, 0.5f);

    glm::vec3 targetToPlayer = playerPos - targetPos;
    targetToPlayer -= up * glm::dot(targetToPlayer, up);

    if (glm::length(targetToPlayer) < 0.001f) {
        targetToPlayer = -player->GetForwardVec();
        targetToPlayer -= up * glm::dot(targetToPlayer, up);
    }

    if (glm::length(targetToPlayer) < 0.001f) {
        targetToPlayer = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));

        if (glm::length(targetToPlayer) < 0.001f) {
            targetToPlayer = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
        }
    }

    const glm::vec3 backDir = glm::normalize(targetToPlayer);

    const float targetDistance = glm::length(playerPos - targetPos);
    const float cameraDistance = glm::clamp(targetDistance + 6.0f, 8.0f, 16.0f);
    constexpr float cameraHeight = 4.0f;

    const glm::vec3 desiredCameraPos = center + backDir * cameraDistance + up * cameraHeight;

    const glm::vec3 lookAtBase = glm::mix(playerPos, targetPos, 0.75f);
    const glm::vec3 desiredLookAt = lookAtBase + up * 1.5f;

    mCameraPos = glm::mix(mCameraPos, desiredCameraPos, cameraLerp);
    mCameraTargetPos = glm::mix(mCameraTargetPos, desiredLookAt, cameraLerp);
    mCameraUpVec = glm::normalize(glm::mix(mCameraUpVec, up, cameraLerp));

    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}

std::vector<glm::mat4> FocusCamera::GetOpeningViews() const
{
    std::vector<glm::mat4> views;

    if (!mGame || !mGame->GetSceneSystem()) {
        return views;
    }

    if (mGame->GetSceneSystem()->IsTalkWithMother()) {
        const glm::mat4 talkWithMotherView =
            glm::lookAt(glm::vec3(-2.0f, 4.0f, -2.0f), glm::vec3(4.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        views.emplace_back(talkWithMotherView);
        return views;
    }

    if (mGame->GetSceneSystem()->IsTalkWithDoctor()) {
        const glm::mat4 talkWithDoctorView =
            glm::lookAt(glm::vec3(3.0f, 4.0f, 1.0f), glm::vec3(-4.0f, 2.0f, -4.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        views.emplace_back(talkWithDoctorView);
        return views;
    }

    return views;
}
