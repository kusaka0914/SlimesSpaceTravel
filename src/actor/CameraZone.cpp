#include "actor/CameraZone.h"

#include "actor/Player.h"
#include "app/Game.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

CameraZone::CameraZone(Game* game)
    : StageObject(game)
{
    SetCollisionEnabled(false);
    SetModelPath("cameraZone.obj");
    SetScale(glm::vec3(6, 6, 6));
}

float CameraZone::GetRenderOpacity() const
{
    return mGame && mGame->GetIsDebugEditorShowing() ? (GetIsEditorSelected() ? 0.28f : 0.08f) : 0.0f;
}

bool CameraZone::Contains(const glm::vec3& point, float worldMargin) const
{
    // MathUtilsのモデル軸補正と同じ入れ替えを適用し、Editorに見える箱と領域を一致させる。
    const glm::vec3 semantic = glm::inverse(GetOrientation()) * (point - GetPos());
    const glm::vec3 local(-semantic.z, semantic.y, -semantic.x);
    const glm::vec3 halfSize = glm::abs(GetScale()) * 0.5f + glm::vec3(std::max(0.0f, worldMargin));
    return glm::all(glm::lessThanEqual(glm::abs(local), halfSize));
}

void CameraZone::CaptureCamera(const CameraPose& pose, const Player& player, bool returnCamera)
{
    CameraZoneViewSettings& settings = returnCamera ? mSettings.returnCamera : mSettings;
    settings.fixedPose = pose;
    settings.fieldOfViewDegrees = pose.fieldOfViewDegrees;
    const auto up = player.GetUpVec();
    settings.referenceUp = glm::dot(up, up) > 0.000001f ? glm::normalize(up) : glm::vec3(0, 1, 0);
    const glm::vec3 forward = pose.target - pose.position;
    const glm::vec3 direction = glm::dot(forward, forward) > 0.000001f ? glm::normalize(forward) : glm::vec3(0, 0, -1);
    glm::vec3 center = player.GetPos();
    if (settings.mode == CameraZoneSettings::Mode::Target) {
        center = glm::mix(center, settings.fallbackTarget, settings.targetWeight);
    }
    const float depth = std::max(0.1f, glm::dot(center - pose.position, direction));
    const glm::vec3 target = pose.position + direction * depth;
    settings.lookOffset = target - center;
    const glm::vec3 offset = pose.position - target;
    settings.distance = glm::length(offset);
    const float vertical = glm::dot(offset, settings.referenceUp);
    settings.backDirection = offset - settings.referenceUp * vertical;
    settings.pitchDegrees =
        glm::degrees(std::asin(std::clamp(vertical / std::max(0.01f, settings.distance), -1.0f, 1.0f)));
    settings.Normalize();
}
