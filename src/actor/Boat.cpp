#include "Boat.h"
#include "Game.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/Planet.h"
#include "component/FocusComponent.h"
#include "system/AudioSystem.h"

Boat::Boat(Game* game)
    : Actor(game),
      mDestPlanet(nullptr),
      mIsMoving(false),
      mIsActivePrev(false),
      mDestStage(0),
      mTransitionTimer(0.0f),
      mProgress(0.0f),
      mStartPos(0.0f),
      mDestPos(0.0f),
      mFocusComponent(nullptr),
      mArrivalPoint(nullptr)
{
    // mIsActive = mGame->IsInBase();
    mIsActive = true;
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

glm::vec3 Boat::CalculateDestPos() const
{
    if (mArrivalPoint) {
        return mArrivalPoint->GetPos();
    }

    if (!mDestPlanet) {
        return mPos;
    }

    const glm::vec3 destPlanetCenter = mDestPlanet->GetPos();
    const glm::vec3 toDestPlanet = glm::normalize(destPlanetCenter - mPos);

    constexpr float destMargin = 4.0f;
    const glm::vec3 destPos = destPlanetCenter - toDestPlanet * (mDestPlanet->GetRadius() + destMargin);
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

    constexpr float transitionDuration = 3.0f;
    const float t = glm::min(1.0f, mTransitionTimer / transitionDuration);
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
        mGame->OnBoatStageChangeRequested(mDestStage);
        return;
    }

    mIsMoving = true;
}