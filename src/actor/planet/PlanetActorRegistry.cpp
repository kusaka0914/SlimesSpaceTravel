#include "actor/planet/PlanetActorRegistry.h"

#include "actor/Platform.h"

void PlanetActorRegistry::RemoveAllPlatforms()
{
    mPlatforms.clear();
}

void PlanetActorRegistry::RemovePlatformsByStageSequence(
    const std::string& sequenceName)
{
    mPlatforms.erase(
        std::remove_if(
            mPlatforms.begin(),
            mPlatforms.end(),
            [&sequenceName](Platform* platform) {
                return platform &&
                       platform->GetStageSequenceName() == sequenceName;
            }),
        mPlatforms.end());
}
