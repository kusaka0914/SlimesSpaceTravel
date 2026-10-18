#pragma once

#include "editor/stage/StageEditorTypes.h"

#include <glm/glm.hpp>

class Actor;
struct DebugEditorContext;

// クリックしたActorの所属惑星と、UGCの配置平面上の位置を解決する。
class StageActorPlacementResolver {
public:
    explicit StageActorPlacementResolver(DebugEditorContext& context);

    // 命中Actorから所属惑星を特定できない場合は fallbackPlanetIndex を返す。
    int ResolvePlanetIndex(Actor* hitActor, int fallbackPlanetIndex) const;

    // 線分 rayFrom〜rayTo と配置面の交点を返す。必要なStageや交点がなければ false。
    bool TryResolveUGCBuildPlanePlacement(const glm::vec3& rayFrom, const glm::vec3& rayTo, int gridLayer,
                                          StageActorPlacement& outPlacement) const;

private:
    DebugEditorContext& mContext;
};
