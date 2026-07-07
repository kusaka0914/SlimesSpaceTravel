#include "actor/Actor.h"
#include "Game.h"
#include "actor/ActorGroundResolver.h"
#include "actor/Planet.h"
#include "component/Component.h"

#include <algorithm>
#include <cmath>

Actor::Actor(Game* game)
    : mGame(game),
      mIsActive(true),
      mIsUpVecInitialized(false),
      mRadius(1.0f),
      mFacingYaw(0.0f),
      mPos(0.0f),
      mUpVec(0.0f, 1.0f, 0.0f),
      mScale(1.0f),
      mCurrentPlanet(nullptr),
      mMeshes(nullptr)
{
}

Actor::~Actor() = default;

void Actor::Initialize() {}

void Actor::ProcessInput()
{
    ProcessActor();
}

void Actor::ProcessActor() {}

void Actor::Update(float deltaTime)
{
    UpdateUpVec();
    UpdateDirectionVectors();

    UpdateActor(deltaTime);

    for (auto& component : mComponents) {
        Component* comp = component.get();
        comp->Update(deltaTime);
    }
}

void Actor::UpdateActor(float deltaTime) {}

void Actor::AddComponent(std::unique_ptr<Component> component)
{
    const int myOrder = component->GetUpdateOrder();
    auto iter = mComponents.begin();
    for (; iter != mComponents.end(); iter++) {
        if (myOrder < (*iter)->GetUpdateOrder()) {
            break;
        }
    }
    mComponents.insert(iter, std::move(component));
}

void Actor::RemoveComponent(std::unique_ptr<Component> component)
{
    const auto iter = std::find(mComponents.begin(), mComponents.end(), component);
    if (iter != mComponents.end()) {
        mComponents.erase(iter);
    }
}

void Actor::UpdateUpVec()
{
    if (!ShouldUpdateUpVecEveryFrame() && mIsUpVecInitialized) {
        return;
    }

    if (glm::length(mUpVec) < 1e-6f) {
        UpdateFallbackUpVec();
    }

    const glm::vec3 avgUpVec = ActorGroundResolver::CalculateAverageNormal(
        mGame, mPos, mUpVec, mForwardVec, mLeftVec,
        [this](const glm::vec3& hitNormal, const glm::vec3& up) { return CheckDotAngleSteep(hitNormal, up); },
        [this]() { OnCastSucceeded(); });

    if (glm::length(avgUpVec) > 1e-6f) {
        mUpVec = avgUpVec;
        mIsUpVecInitialized = true;
        return;
    }

    OnUpVecUpdateFailed();
}

void Actor::UpdateDirectionVectors()
{
    glm::vec3 upN = mUpVec;
    if (glm::length(upN) < 1e-6f) {
        upN = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    upN = glm::normalize(upN);

    glm::vec3 baseLeft = glm::cross(upN, glm::vec3(0.0f, 0.0f, 1.0f));
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(upN, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::cross(upN, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    baseLeft = glm::normalize(baseLeft);
    glm::vec3 baseForward = glm::cross(baseLeft, upN);

    mForwardVec = glm::normalize(baseForward * std::cos(mFacingYaw) - baseLeft * std::sin(mFacingYaw));

    mLeftVec = glm::normalize(glm::cross(upN, mForwardVec));
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
