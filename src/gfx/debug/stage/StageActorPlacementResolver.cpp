#include "gfx/debug/stage/StageActorPlacementResolver.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorContext.h"

#include <algorithm>
#include <cmath>

StageActorPlacementResolver::StageActorPlacementResolver(
    DebugEditorContext& context)
    : mContext(context)
{
}

int StageActorPlacementResolver::ResolvePlanetIndex(
    Actor* hitActor,
    int fallbackPlanetIndex) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return fallbackPlanetIndex;
    }

    Planet* hitPlanet = dynamic_cast<Planet*>(hitActor);
    if (!hitPlanet && hitActor) {
        hitPlanet = hitActor->GetCurrentPlanet();
    }
    if (!hitPlanet) {
        return fallbackPlanetIndex;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const auto planetIt = std::find(planets.begin(), planets.end(), hitPlanet);
    if (planetIt == planets.end()) {
        return fallbackPlanetIndex;
    }
    return static_cast<int>(std::distance(planets.begin(), planetIt));
}

bool StageActorPlacementResolver::TryResolveUGCBuildPlanePlacement(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo,
    int gridLayer,
    StageActorPlacement& outPlacement) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty() || !planets.front()) {
        return false;
    }

    const glm::vec3 rayDelta = rayTo - rayFrom;
    constexpr float parallelEpsilon = 0.00001f;
    float rayParameter = -1.0f;
    const float buildPlaneY =
        static_cast<float>(gridLayer) *
        mContext.game->GetUGCGridSize();

    // UGCの組立面は、重なった足場ではなく選択中レイヤーの床に固定する。
    if (std::abs(rayDelta.y) > parallelEpsilon) {
        rayParameter =
            (buildPlaneY - rayFrom.y) / rayDelta.y;
    }

    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        // 横向きの視線では、同じ惑星中心を通るカメラ正面の面を代わりに使う。
        const glm::vec3 buildPlaneCenter(
            planets.front()->GetPos().x,
            buildPlaneY,
            planets.front()->GetPos().z);
        glm::vec3 viewPlaneNormal = rayFrom - buildPlaneCenter;
        const float normalLength = glm::length(viewPlaneNormal);
        if (normalLength <= parallelEpsilon) {
            return false;
        }
        viewPlaneNormal /= normalLength;

        const float denominator = glm::dot(rayDelta, viewPlaneNormal);
        if (std::abs(denominator) <= parallelEpsilon) {
            return false;
        }
        rayParameter = glm::dot(
            buildPlaneCenter - rayFrom,
            viewPlaneNormal) / denominator;
    }

    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        return false;
    }

    outPlacement.worldPosition = rayFrom + rayDelta * rayParameter;
    outPlacement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    return true;
}
