#pragma once

#include <string>

class PlanetActorRegistry;

class PlanetProgressController {
public:
    enum class RocketSpawnCondition { AllEnemiesDead, AllBoatPartsCollected, None };

    PlanetProgressController();

    void Initialize(const PlanetActorRegistry& actorRegistry);

    void OnEnemyDead(const PlanetActorRegistry& actorRegistry);
    void OnBoatPartsObtained(const PlanetActorRegistry& actorRegistry);

    void SetRocketSpawnCondition(const std::string& rocketSpawnCondition);
    std::string GetRocketSpawnCondition() const;

    int GetRemainBoatPartsCount() const { return mRemainBoatPartsCount; }

private:
    void InitRemainBoatPartsCount(const PlanetActorRegistry& actorRegistry);

    bool CheckIsAllEnemiesDead(const PlanetActorRegistry& actorRegistry) const;
    bool CheckIsAllBoatPartsCollected(const PlanetActorRegistry& actorRegistry) const;

    void StartBoatFocus(const PlanetActorRegistry& actorRegistry) const;

private:
    int mRemainBoatPartsCount;

    RocketSpawnCondition mRocketSpawnCondition;
};
