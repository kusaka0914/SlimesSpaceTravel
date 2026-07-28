#include "actor/Actor.h"

#include "Game.h"
#include "actor/ActorGroundResolver.h"
#include "actor/Planet.h"
#include "component/Component.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>
#include <utility>

Actor::Actor(Game* game)
    : mIsActive(true),
      mIsUpVecInitialized(false),
      mRadius(1.0f),
      mFacingYaw(0.0f),
      mPos(0.0f),
      mUpVec(0.0f, 1.0f, 0.0f),
      mScale(1.0f),
      mGame(game),
      mCurrentPlanet(nullptr),
      mLoadedModel(nullptr)
{
}

Actor::~Actor() = default;

void Actor::Initialize() {}

void Actor::ProcessInput()
{
    if (!IsProgressVisibleForCurrentMode()) {
        return;
    }
    ProcessActor();
}

void Actor::ProcessActor() {}

void Actor::Update(float deltaTime)
{
    if (!IsProgressVisibleForCurrentMode()) {
        return;
    }

    UpdateUpVec();
    UpdateDirectionVectors();

    UpdateActor(deltaTime);

    for (auto& component : mComponents) {
        Component* componentPtr = component.get();
        componentPtr->Update(deltaTime);
    }
}

void Actor::UpdateActor(float deltaTime) {}

void Actor::AddComponent(std::unique_ptr<Component> component)
{
    const int updateOrder = component->GetUpdateOrder();
    auto insertPosition = mComponents.begin();
    for (; insertPosition != mComponents.end(); ++insertPosition) {
        if (updateOrder < (*insertPosition)->GetUpdateOrder()) {
            break;
        }
    }
    mComponents.insert(insertPosition, std::move(component));
}

void Actor::RemoveComponent(std::unique_ptr<Component> component)
{
    const auto componentIt = std::find(mComponents.begin(), mComponents.end(), component);
    if (componentIt != mComponents.end()) {
        mComponents.erase(componentIt);
    }
}

void Actor::SetLoadedModel(const LoadedModel* loadedModel)
{
    mLoadedModel = loadedModel;
    OnLoadedModelChanged();
}

void Actor::SetVisibleIfStageCleared(int stageNum)
{
    mVisibleIfStageCleared = stageNum >= 0 ? stageNum : -1;
    RefreshProgressVisibility();
}

void Actor::RefreshProgressVisibility()
{
    mProgressVisibilitySatisfied =
        mVisibleIfStageCleared < 0 ||
        (mGame && mGame->IsStageCleared(mVisibleIfStageCleared));
}

const std::vector<LoadedMesh>* Actor::GetMeshes() const
{
    return mLoadedModel ? &mLoadedModel->meshes : nullptr;
}

void Actor::UpdateUpVec()
{
    if (!ShouldUpdateUpVecEveryFrame() && mIsUpVecInitialized) {
        return;
    }

    if (glm::length(mUpVec) < 1e-6f) {
        UpdateFallbackUpVec();
    }

    const glm::vec3 averageUpVec = ActorGroundResolver::CalculateAverageNormal(
        mGame, mPos, mUpVec, mForwardVec, mLeftVec,
        [this](const glm::vec3& hitNormal, const glm::vec3& up) { return CheckDotAngleSteep(hitNormal, up); },
        [this]() { OnCastSucceeded(); });

    if (glm::length(averageUpVec) > 1e-6f) {
        mUpVec = averageUpVec;
        mIsUpVecInitialized = true;
        return;
    }

    OnUpVecUpdateFailed();
}

void Actor::UpdateDirectionVectors()
{
    glm::vec3 normalizedUp = mUpVec;
    if (glm::length(normalizedUp) < 1e-6f) {
        normalizedUp = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    normalizedUp = glm::normalize(normalizedUp);

    glm::vec3 baseLeft = glm::cross(normalizedUp, glm::vec3(0.0f, 0.0f, 1.0f));
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(normalizedUp, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(normalizedUp, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    baseLeft = glm::normalize(baseLeft);
    const glm::vec3 baseForward = glm::cross(baseLeft, normalizedUp);

    mForwardVec = glm::normalize(baseForward * std::cos(mFacingYaw) - baseLeft * std::sin(mFacingYaw));
    mLeftVec = glm::normalize(glm::cross(normalizedUp, mForwardVec));
}

void Actor::OnUpVecUpdateFailed()
{
    UpdateFallbackUpVec();
    mIsUpVecInitialized = true;
}

void Actor::UpdateFallbackUpVec()
{
    mUpVec = ActorGroundResolver::CalculateFallbackUpVec(mCurrentPlanet, mPos);
}
