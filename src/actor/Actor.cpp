#include "actor/Actor.h"
#include "Game.h"
#include "actor/Planet.h"
#include "component/Component.h"
#include "system/PhysicsSystem.h"

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

    const glm::vec3 avgUpVec = GetAverageNormal();

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
    if (!mCurrentPlanet) {
        mUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }

    bool isNormalShape = mCurrentPlanet->GetPlanetShape() == Planet::PlanetShape::Normal;
    if (isNormalShape) {
        mUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }

    const glm::vec3 toActor = mPos - mCurrentPlanet->GetPos();

    if (glm::length(toActor) < 1e-6f) {
        mUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }

    mUpVec = glm::normalize(toActor);
}

glm::vec3 Actor::GetAverageNormal()
{
    glm::vec3 mainNormal;
    const btCollisionObject* mainObj = nullptr;

    if (!CastRay(glm::vec3(0.0f), mainNormal, mainObj)) {
        return glm::vec3(0.0f);
    }

    constexpr float mainWeight = 3.0f;
    glm::vec3 normalSum = mainNormal * mainWeight;
    float weightSum = mainWeight;

    constexpr float footRadius = 0.25f;
    const std::vector<glm::vec3> offsets = {mForwardVec * footRadius, -mForwardVec * footRadius, mLeftVec * footRadius,
                                            -mLeftVec * footRadius};

    for (const auto& offset : offsets) {
        glm::vec3 hitNormal;
        const btCollisionObject* hitObj = nullptr;

        if (!CastRay(offset, hitNormal, hitObj))
            continue;

        if (hitObj != mainObj)
            continue;

        normalSum += hitNormal;
        weightSum += 1.0f;
    }

    const glm::vec3 averageNormal = normalSum / weightSum;

    if (glm::length(averageNormal) < 1e-6f) {
        return glm::vec3(0.0f);
    }

    return glm::normalize(averageNormal);
}

bool Actor::CastRay(const glm::vec3& offset, glm::vec3& outNormal, const btCollisionObject*& outObj)
{
    if (glm::length(mUpVec) < 1e-6f) {
        UpdateFallbackUpVec();
    }

    const glm::vec3 up = glm::normalize(mUpVec);
    constexpr float rayStartOffset = 0.2f;
    constexpr float rayLength = 30.0f;

    const glm::vec3 downRayFromPos = mPos + offset + up * rayStartOffset;
    const glm::vec3 downRayToPos = mPos + offset - up * rayLength;

    const btVector3 downRayFrom(downRayFromPos.x, downRayFromPos.y, downRayFromPos.z);
    const btVector3 downRayTo(downRayToPos.x, downRayToPos.y, downRayToPos.z);

    btCollisionWorld::ClosestRayResultCallback cb(downRayFrom, downRayTo);

    const btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();

    bulletWorld->rayTest(downRayFrom, downRayTo, cb);

    if (!cb.hasHit()) {
        return false;
    }

    btVector3 hitN;
    const btCollisionObject* chosenObj = nullptr;

    if (cb.hasHit()) {
        hitN = cb.m_hitNormalWorld;
        chosenObj = cb.m_collisionObject;
    }

    glm::vec3 hitNormal(hitN.x(), hitN.y(), hitN.z());
    if (glm::length(hitNormal) < 1e-6f)
        return false;

    hitNormal = glm::normalize(hitNormal);

    if (CheckDotAngleSteep(hitNormal, up)) {
        return false;
    }

    OnCastSucceeded();

    outNormal = hitNormal;
    outObj = chosenObj;
    return true;
}