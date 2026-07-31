#include "actor/TutorialTrigger.h"

#include "Game.h"
#include "actor/Player.h"
#include "system/MeshLoadSystem.h"
#include "system/SceneSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <vector>

namespace {
glm::mat4 CreateModelOrientation(
    const TutorialTrigger& trigger)
{
    const glm::mat4 semanticOrientation =
        glm::mat4_cast(trigger.GetOrientation());

    glm::mat4 modelAxisCorrection(1.0f);
    modelAxisCorrection[0] =
        glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    modelAxisCorrection[1] =
        glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    modelAxisCorrection[2] =
        glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);

    return semanticOrientation * modelAxisCorrection;
}
}

TutorialTrigger::TutorialTrigger(Game* game)
    : NPC(game)
{
    SetModelPath("selectField.obj");
    SetName("Tutorial Trigger");
    SetScale(glm::vec3(2.0f));
}

float TutorialTrigger::GetRenderOpacity() const
{
    if (!mGame || !mGame->GetIsDebugEditorShowing()) {
        return 0.0f;
    }
    return GetIsEditorSelected() ? 0.22f : 0.08f;
}

void TutorialTrigger::OnLoadedModelChanged()
{
    mLocalBoundsMin = glm::vec3(-0.5f);
    mLocalBoundsMax = glm::vec3(0.5f);

    if (!mGame || !mGame->GetMeshLoadSystem() ||
        GetModelPath().empty()) {
        return;
    }

    std::filesystem::path modelPath(GetModelPath());
    if (!modelPath.is_absolute()) {
        modelPath =
            std::filesystem::path("../assets/models") /
            modelPath;
    }

    std::vector<float> positions;
    std::vector<unsigned int> indices;
    if (!mGame->GetMeshLoadSystem()
             ->LoadMeshPositionsAndIndices(
                 modelPath.lexically_normal().string().c_str(),
                 positions,
                 indices) ||
        positions.size() < 3) {
        return;
    }

    glm::vec3 boundsMin(
        std::numeric_limits<float>::max());
    glm::vec3 boundsMax(
        std::numeric_limits<float>::lowest());
    for (std::size_t index = 0;
         index + 2 < positions.size();
         index += 3) {
        const glm::vec3 position(
            positions[index],
            positions[index + 1],
            positions[index + 2]);
        boundsMin = glm::min(boundsMin, position);
        boundsMax = glm::max(boundsMax, position);
    }

    constexpr float minimumBoundsSize = 0.0001f;
    if (glm::length(boundsMax - boundsMin) <=
        minimumBoundsSize) {
        return;
    }

    mLocalBoundsMin = boundsMin;
    mLocalBoundsMax = boundsMax;
}

bool TutorialTrigger::IsInsideModelBounds(
    const glm::vec3& worldPosition) const
{
    glm::vec3 safeScale = glm::abs(GetScale());
    safeScale.x = std::max(0.0001f, safeScale.x);
    safeScale.y = std::max(0.0001f, safeScale.y);
    safeScale.z = std::max(0.0001f, safeScale.z);

    const glm::mat4 modelMatrix =
        glm::translate(glm::mat4(1.0f), GetPos()) *
        CreateModelOrientation(*this) *
        glm::scale(glm::mat4(1.0f), safeScale);
    const glm::vec3 localPosition =
        glm::vec3(
            glm::inverse(modelMatrix) *
            glm::vec4(worldPosition, 1.0f));

    constexpr float boundsEpsilon = 0.0001f;
    return localPosition.x >=
               mLocalBoundsMin.x - boundsEpsilon &&
           localPosition.x <=
               mLocalBoundsMax.x + boundsEpsilon &&
           localPosition.y >=
               mLocalBoundsMin.y - boundsEpsilon &&
           localPosition.y <=
               mLocalBoundsMax.y + boundsEpsilon &&
           localPosition.z >=
               mLocalBoundsMin.z - boundsEpsilon &&
           localPosition.z <=
               mLocalBoundsMax.z + boundsEpsilon;
}

void TutorialTrigger::UpdateActor(float)
{
    if (mHasTriggeredThisVisit || !mGame ||
        mGame->GetIsDebugEditorShowing()) {
        return;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsPlaying()) {
        return;
    }

    for (Player* player : mGame->GetPlayers()) {
        if (!player || !player->GetIsActive() ||
            !player->GetOnGround() ||
            player->GetCurrentPlanet() != GetCurrentPlanet()) {
            continue;
        }

        if (!IsInsideModelBounds(player->GetPos())) {
            continue;
        }

        if (!mTutorialId.empty()) {
            mHasTriggeredThisVisit =
                sceneSystem->TryStartTutorial(
                    mTutorialId,
                    player);
        } else {
            mHasTriggeredThisVisit = true;
            sceneSystem->StartTalkWithNPC(this, player);
        }
        return;
    }
}
