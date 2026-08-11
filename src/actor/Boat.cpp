#include "Boat.h"
#include "Game.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/Planet.h"
#include "component/FocusComponent.h"
#include "system/AudioSystem.h"
#include "system/sequence/SequenceSystem.h"

#include <algorithm>

namespace {
constexpr const char* BaseLaunchSequenceId = "launch_rocket_from_base";
}

Boat::Boat(Game* game)
    : Actor(game),
      mDestPlanet(nullptr),
      mIsMoving(false),
      mIsActivePrev(false),
      mDestStage(0),
      mProgress(0.0f),
      mTravelSpeed(10.0f),
      mTravelDistance(0.0f),
      mTravelledDistance(0.0f),
      mDestMargin(4.0f),
      mStartPos(0.0f),
      mDestPos(0.0f),
      mLaunchSequenceId(BaseLaunchSequenceId),
      mFocusComponent(nullptr),
      mArrivalPoint(nullptr)
{
    mIsActive = mGame->IsInBase();
    // mIsActive = true;
    AddFocusComponent();
}

void Boat::AddFocusComponent()
{
    std::unique_ptr<FocusComponent> focusComponent = std::make_unique<FocusComponent>(this, 100);
    mFocusComponent = focusComponent.get();
    AddComponent(std::move(focusComponent));
}

void Boat::Initialize()
{
    mStartPos = mPos;
    mDestPos = CalculateDestPos();
}

void Boat::SetDestPlanet(Planet* destPlanet)
{
    mDestPlanet = destPlanet;
    RefreshDestination();
}

void Boat::SetArrivalPoint(BoatArrivalPoint* arrivalPoint)
{
    mArrivalPoint = arrivalPoint;
    RefreshDestination();
}

void Boat::SetTravelSpeed(float travelSpeed)
{
    mTravelSpeed = std::max(0.1f, travelSpeed);
}

void Boat::SetTravelSpeedFromLegacyDuration(float travelDuration)
{
    const float safeDuration = std::max(0.1f, travelDuration);
    const float travelDistance = glm::length(mDestPos - mPos);
    if (travelDistance <= 0.000001f) {
        return;
    }

    SetTravelSpeed(travelDistance / safeDuration);
}

void Boat::SetDestMargin(float destMargin)
{
    mDestMargin = std::max(0.0f, destMargin);
    RefreshDestination();
}

void Boat::RefreshDestination()
{
    if (!mIsMoving) {
        mDestPos = CalculateDestPos();
    }
}

BoatArrivalPoint* Boat::ResolveArrivalPoint() const
{
    if (mArrivalPoint && mArrivalPoint->GetIsActive()) {
        return mArrivalPoint;
    }

    if (!mDestPlanet) {
        return nullptr;
    }

    BoatArrivalPoint* uniqueArrivalPoint = nullptr;
    for (BoatArrivalPoint* arrivalPoint :
         mDestPlanet->GetBoatArrivalPoints()) {
        if (!arrivalPoint || !arrivalPoint->GetIsActive()) {
            continue;
        }

        if (uniqueArrivalPoint) {
            return nullptr;
        }
        uniqueArrivalPoint = arrivalPoint;
    }
    return uniqueArrivalPoint;
}

glm::vec3 Boat::CalculateDestPos() const
{
    if (BoatArrivalPoint* arrivalPoint = ResolveArrivalPoint()) {
        return arrivalPoint->GetPos();
    }

    if (!mDestPlanet) {
        return mPos;
    }

    const glm::vec3 destPlanetCenter = mDestPlanet->GetPos();
    glm::vec3 toDestPlanet = destPlanetCenter - mPos;
    if (glm::length(toDestPlanet) < 1e-6f) {
        toDestPlanet = glm::vec3(0.0f, -1.0f, 0.0f);
    } else {
        toDestPlanet = glm::normalize(toDestPlanet);
    }

    const glm::vec3 destPos =
        destPlanetCenter -
        toDestPlanet * (mDestPlanet->GetRadius() + mDestMargin);
    return destPos;
}

void Boat::UpdateActor(float deltaTime)
{
    const bool isInStage = !mGame->IsInBase();
    const bool isJustShown = !mIsActivePrev && mIsActive;
    if (isInStage && isJustShown) {
        OnShown();
    }

    if (mIsMoving) {
        UpdateMoving(deltaTime);
    }

    mIsActivePrev = mIsActive;
}

void Boat::StartFocus()
{
    mFocusComponent->StartFocus();
}

void Boat::OnShown() const
{
    mGame->GetAudioSystem()->PlaySE("show_boat_se");
}

void Boat::UpdateMoving(float deltaTime)
{
    UpdateMovement(deltaTime);
    if (mProgress >= 1.0f) {
        FinishMoving();
    }
}

void Boat::UpdateMovement(float deltaTime)
{
    if (mTravelDistance <= 0.000001f) {
        mProgress = 1.0f;
        mPos = mDestPos;
        return;
    }

    const float travelStep =
        mTravelSpeed * std::max(0.0f, deltaTime);
    mTravelledDistance =
        std::min(mTravelDistance, mTravelledDistance + travelStep);
    mProgress = mTravelledDistance / mTravelDistance;
    mPos = glm::mix(mStartPos, mDestPos, mProgress);
}

void Boat::FinishMoving()
{
    mPos = mDestPos;
    mIsMoving = false;
    mIsActive = false;

    mGame->OnBoatArrived(this);
}

void Boat::StartTravel()
{
    if (mGame->IsInBase()) {
        SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
        if (sequenceSystem && !mLaunchSequenceId.empty()) {
            sequenceSystem->Play(mLaunchSequenceId);
        }

        mGame->OnBoatStageChangeRequested(mDestStage);
        return;
    }

    mStartPos = mPos;
    mDestPos = CalculateDestPos();
    mTravelDistance = glm::length(mDestPos - mStartPos);
    mTravelledDistance = 0.0f;
    mProgress = 0.0f;
    mIsMoving = true;
}
