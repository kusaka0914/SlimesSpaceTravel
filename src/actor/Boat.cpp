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
      mTransitionTimer(0.0f),
      mProgress(0.0f),
      mTravelDuration(3.0f),
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

void Boat::SetTravelDuration(float travelDuration)
{
    mTravelDuration = std::max(0.1f, travelDuration);
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

glm::vec3 Boat::CalculateDestPos() const
{
    if (mArrivalPoint) {
        return mArrivalPoint->GetPos();
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
    const bool hasArrived = mProgress >= 1.0f;
    if (hasArrived) {
        FinishMoving();
        return;
    }

    UpdateMovement(deltaTime);
}

void Boat::UpdateMovement(float deltaTime)
{
    mTransitionTimer += deltaTime;

    const float t = glm::min(1.0f, mTransitionTimer / mTravelDuration);
    mProgress = glm::smoothstep(0.0f, 1.0f, t);

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
    mTransitionTimer = 0.0f;
    mProgress = 0.0f;
    mIsMoving = true;
}
