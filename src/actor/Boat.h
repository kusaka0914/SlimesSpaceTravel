#pragma once

#include "actor/Actor.h"
#include <glm/glm.hpp>
#include <string>

class Game;
class FocusComponent;
class BoatArrivalPoint;

class Boat : public Actor {
public:
    Boat(Game* game);
    void Initialize() override;
    void UpdateActor(float deltaTime) override;

    void StartTravel();
    void StartFocus();

    void SetDestPlanet(Planet* destPlanet);
    void SetDestStage(int destStage) { mDestStage = destStage; }
    void SetArrivalPoint(BoatArrivalPoint* arrivalPoint);
    void SetTravelSpeed(float travelSpeed);
    void SetTravelSpeedFromLegacyDuration(float travelDuration);
    void SetDestMargin(float destMargin);
    void SetLaunchSequenceId(const std::string& sequenceId) { mLaunchSequenceId = sequenceId; }
    void RefreshDestination();

    bool GetIsMoving() const { return mIsMoving; }
    bool HasAppeared() const { return mIsActive; }

    float GetProgress() const { return mProgress; }
    float GetTravelSpeed() const { return mTravelSpeed; }
    float GetDestMargin() const { return mDestMargin; }
    int GetDestStage() const { return mDestStage; }
    const glm::vec3& GetDestPos() const { return mDestPos; }
    const std::string& GetLaunchSequenceId() const { return mLaunchSequenceId; }
    Planet* GetDestPlanet() const { return mDestPlanet; }
    FocusComponent* GetFocusComponent() const { return mFocusComponent; }
    BoatArrivalPoint* GetArrivalPoint() const { return mArrivalPoint; }

private:
    void AddFocusComponent();

    void OnShown() const;

    void UpdateMoving(float deltaTime);
    void UpdateMovement(float deltaTime);

    void FinishMoving();

    glm::vec3 CalculateDestPos() const;

private:
    Planet* mDestPlanet;

    bool mIsMoving;
    bool mIsActivePrev;

    int mDestStage;

    float mProgress;
    float mTravelSpeed;
    float mTravelDistance;
    float mTravelledDistance;
    float mDestMargin;

    glm::vec3 mStartPos;
    glm::vec3 mDestPos;

    std::string mLaunchSequenceId;

    FocusComponent* mFocusComponent;
    BoatArrivalPoint* mArrivalPoint;
};
