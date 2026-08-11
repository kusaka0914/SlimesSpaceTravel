#include "actor/Actor.h"

#include "Game.h"
#include "actor/ActorGroundResolver.h"
#include "actor/Boat.h"
#include "actor/Planet.h"
#include "component/Component.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
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
    UpdateDirectionVectors();
}

Actor::~Actor() = default;

void Actor::Initialize() {}

void Actor::ProcessInput()
{
    if (mIsDebugDisabled ||
        !IsProgressVisibleForCurrentMode() ||
        !IsRuntimeActivationEnabledForCurrentMode()) {
        return;
    }
    ProcessActor();
}

void Actor::ProcessActor() {}

void Actor::Update(float deltaTime)
{
    if (mIsDebugDisabled ||
        !IsProgressVisibleForCurrentMode() ||
        !IsRuntimeActivationEnabledForCurrentMode()) {
        return;
    }

    UpdateUpVec();
    if (ShouldRebuildDirectionVectorsEveryFrame()) {
        UpdateDirectionVectors();
    }

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

void Actor::RemoveComponent(Component* component)
{
    const auto componentIt =
        std::find_if(
            mComponents.begin(),
            mComponents.end(),
            [component](const std::unique_ptr<Component>& current) {
                return current.get() == component;
            });
    if (componentIt != mComponents.end()) {
        mComponents.erase(componentIt);
    }
}

void Actor::SetRuntimeActivationEnabled(
    const Component* source,
    bool isEnabled)
{
    if (!source) {
        return;
    }
    mRuntimeActivationStates[source] = isEnabled;
}

void Actor::ClearRuntimeActivationState(const Component* source)
{
    if (!source) {
        return;
    }
    mRuntimeActivationStates.erase(source);
}

bool Actor::IsRuntimeActivationEnabledForCurrentMode() const
{
    if (mGame && mGame->GetIsDebugEditorShowing()) {
        return true;
    }

    for (const auto& [source, isEnabled] :
         mRuntimeActivationStates) {
        (void)source;
        if (!isEnabled) {
            return false;
        }
    }
    return true;
}

void Actor::SetLoadedModel(const LoadedModel* loadedModel)
{
    mLoadedModel = loadedModel;
    OnLoadedModelChanged();
}

void Actor::SetFacingYaw(float facingYaw)
{
    mFacingYaw = facingYaw;
    UpdateDirectionVectors();
}

void Actor::SetUpVec(const glm::vec3& upVec)
{
    mUpVec = upVec;
    UpdateDirectionVectors();
}

void Actor::SetOrientation(const glm::quat& orientation)
{
    if (glm::length(orientation) < 1e-6f) {
        return;
    }

    mOrientation = glm::normalize(orientation);
    mLeftVec = glm::normalize(mOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
    mUpVec = glm::normalize(mOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
    mForwardVec = glm::normalize(mOrientation * glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 baseLeft = glm::cross(mUpVec, glm::vec3(0.0f, 0.0f, 1.0f));
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(mUpVec, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(mUpVec, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    baseLeft = glm::normalize(baseLeft);

    const glm::vec3 baseForward = glm::normalize(glm::cross(baseLeft, mUpVec));
    mFacingYaw = std::atan2(-glm::dot(mForwardVec, baseLeft), glm::dot(mForwardVec, baseForward));
}

void Actor::SetVisibleIfStageCleared(int stageNum)
{
    mVisibleIfStageCleared = stageNum >= 0 ? stageNum : -1;
    if (mVisibleIfStageCleared >= 0) {
        mHiddenIfStageCleared = -1;
    }
    RefreshProgressVisibility();
}

void Actor::SetHiddenIfStageCleared(int stageNum)
{
    mHiddenIfStageCleared = stageNum >= 0 ? stageNum : -1;
    if (mHiddenIfStageCleared >= 0) {
        mVisibleIfStageCleared = -1;
    }
    RefreshProgressVisibility();
}

void Actor::RefreshProgressVisibility()
{
    const bool visibleConditionSatisfied =
        mVisibleIfStageCleared < 0 ||
        (mGame && mGame->IsStageCleared(mVisibleIfStageCleared));
    const bool hiddenConditionSatisfied =
        mHiddenIfStageCleared < 0 ||
        !(mGame && mGame->IsStageCleared(mHiddenIfStageCleared));
    mStageClearVisibilitySatisfied =
        visibleConditionSatisfied && hiddenConditionSatisfied;
}

bool Actor::IsProgressVisibilitySatisfied() const
{
    const bool isHiddenByRocket =
        mHiddenWhenRocketAppears &&
        dynamic_cast<const Boat*>(this) == nullptr &&
        mCurrentPlanet &&
        mCurrentPlanet->HasAppearedRocket();
    return mStageClearVisibilitySatisfied &&
           !isHiddenByRocket;
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
        [this]() { OnGroundSurfaceDetected(); },
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
    mUpVec = normalizedUp;
    UpdateOrientationFromDirectionVectors();
}

void Actor::UpdateOrientationFromDirectionVectors()
{
    glm::mat3 orientationMatrix(1.0f);
    orientationMatrix[0] = mLeftVec;
    orientationMatrix[1] = mUpVec;
    orientationMatrix[2] = mForwardVec;
    mOrientation = glm::normalize(glm::quat_cast(orientationMatrix));
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
