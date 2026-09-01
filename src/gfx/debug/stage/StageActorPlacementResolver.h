#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>

class Actor;
struct DebugEditorContext;

class StageActorPlacementResolver {
public:
    explicit StageActorPlacementResolver(DebugEditorContext& context);

    int ResolvePlanetIndex(
        Actor* hitActor,
        int fallbackPlanetIndex) const;
    bool TryResolveUGCBuildPlanePlacement(
        const glm::vec3& rayFrom,
        const glm::vec3& rayTo,
        int gridLayer,
        StageActorPlacement& outPlacement) const;

private:
    DebugEditorContext& mContext;
};
