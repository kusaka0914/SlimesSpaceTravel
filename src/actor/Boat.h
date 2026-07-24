#pragma once

#include "actor/Actor.h"
#include <glm/glm.hpp>

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

    void SetDestPlanet(Planet* destPlanet) { mDestPlanet = destPlanet; }
    void SetDestStage(int destStage) { mDestStage = destStage; }
    void SetArrivalPoint(BoatArrivalPoint* arrivalPoint) { mArrivalPoint = arrivalPoint; }

    bool GetIsMoving() const { return mIsMoving; }

    float GetProgress() const { return mProgress; }
    const glm::vec3& GetDestPos() const { return mDestPos; }
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

    float mTransitionTimer;
    float mProgress;

    glm::vec3 mStartPos;
    glm::vec3 mDestPos;

    FocusComponent* mFocusComponent;
    BoatArrivalPoint* mArrivalPoint;
};
