#include "gfx/debug/panels/CameraDebugPanel.h"

#include "Game.h"
#include "imgui.h"
#include "system/CameraSystem.h"

CameraDebugPanel::CameraDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void CameraDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();

    if (ImGui::CollapsingHeader("カメラ")) {
        const glm::vec3 cameraPos = cameraSystem->GetCameraPos();

        ImGui::Text("位置");
        ImGui::Text("X: %.2f", cameraPos.x);
        ImGui::Text("Y: %.2f", cameraPos.y);
        ImGui::Text("Z: %.2f", cameraPos.z);
    }
}