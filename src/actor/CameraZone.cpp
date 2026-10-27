#include "actor/CameraZone.h"

#include "app/Game.h"
#include <algorithm>
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
