#include "gfx/render3d/RenderViewportController.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "system/CameraSystem.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

RenderViewportController::RenderViewportController(Game* game, const Renderer3D* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void RenderViewportController::DrawGameScreen(float fbWidth, float fbHeight) const
{
    if (!mGame || !mRenderer) {
        return;
    }

    if (!mGame->GetIsPlayer2Joined()) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight);
        return;
    }

    DrawGameScreenForMultiPerson(fbWidth, fbHeight);
}

void RenderViewportController::DrawGameScreenForSinglePerson(float fbWidth, float fbHeight) const
{
    glViewport(0, 0, static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(fbHeight));

    CameraSystem* cameraSystem = mGame->GetCameraSystem();
    if (!cameraSystem || fbHeight <= 0.0f) {
        return;
    }

    const float aspect = fbWidth / fbHeight;
    const float fieldOfViewDegrees = cameraSystem->GetFieldOfViewDegrees();
    const glm::mat4 proj = glm::perspective(glm::radians(fieldOfViewDegrees), aspect, 0.1f, 100.0f);

    std::vector<glm::mat4> views = cameraSystem->GetViews();
    if (views.empty()) {
        return;
    }

    const glm::vec3 cameraPos = cameraSystem->GetCameraPos();
    mRenderer->DrawScene(views[0], proj, cameraPos);
}

void RenderViewportController::DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const
{
    std::vector<glm::mat4> views = mGame->GetCameraSystem()->GetViews();

    if (views.size() < 2) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight);
        return;
    }

    const float halfHeight = fbHeight * 0.5f;
    const float aspect = fbWidth / halfHeight;
    const float fieldOfViewDegrees = mGame->GetCameraSystem()->GetFieldOfViewDegrees();
    const glm::mat4 proj = glm::perspective(glm::radians(fieldOfViewDegrees), aspect, 0.1f, 100.0f);

    const glm::vec3 p1CameraPos = mGame->GetCameraSystem()->GetPlayerCameraPos(0);
    const glm::vec3 p2CameraPos = mGame->GetCameraSystem()->GetPlayerCameraPos(1);

    glViewport(0, static_cast<GLint>(halfHeight), static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    mRenderer->DrawScene(views[0], proj, p1CameraPos);

    glViewport(0, 0, static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    mRenderer->DrawScene(views[1], proj, p2CameraPos);
}
