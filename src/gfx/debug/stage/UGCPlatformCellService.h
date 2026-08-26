#pragma once

#include "gfx/debug/stage/StageActorNodeFactory.h"
#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct DebugEditorContext;

class UGCPlatformCellService {
public:
    explicit UGCPlatformCellService(DebugEditorContext& context);

    bool AddCell(
        int planetIndex,
        const StageActorPlacement& placement,
        float gridSize,
        int footprintSideLength = 1,
        const std::string& behavior = "normal",
        const glm::ivec3& movementDeltaCells = glm::ivec3(0));
    bool RefreshGeneratedPlatforms();
    bool TranslateCells(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& worldDelta);
    bool TranslateCells(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta);
    bool RemoveCell(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& hitPosition);
    bool RemoveCellAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int gridLayer);
    bool ResolveLayerAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int preferredGridLayer,
        int& outGridLayer) const;
    int ResolvePlacementLayerAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int emptyColumnGridLayer) const;

private:
    bool CanEditCells() const;
    bool IsValidPlanetIndex(int planetIndex) const;
    bool RebuildGeneratedPlatforms(YAML::Node& stageConfig) const;
    void ApplyPlacement(
        YAML::Node& actorNode,
        int planetIndex,
        const StageActorPlacement& placement) const;

    DebugEditorContext& mContext;
    StageActorNodeFactory mNodeFactory;
};
