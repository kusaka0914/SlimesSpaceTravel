#pragma once

#include "gfx/debug/stage/StageActorNodeFactory.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/stage/UGCPlatformDocument.h"

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
    bool AddCells(
        int planetIndex,
        const std::vector<glm::ivec3>& anchorGridPositions,
        float gridSize,
        int footprintSideLength,
        const std::string& behavior,
        const glm::ivec3& movementDeltaCells = glm::ivec3(0));
    bool RefreshGeneratedPlatforms();
    bool TranslateCells(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& worldDelta);
    bool TranslateCells(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta);
    bool TranslateMovingPlatformDestinations(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta);
    bool SaveMovingPlatformDestinationTranslation(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta);
    bool RemoveMovingPlatformDestinationCell(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& destinationWorldPosition);
    bool ResolveMovingPlatformRegion(
        const StageActorRef& generatedPlatformRef,
        UGCGeneratedPlatformRegion& outRegion) const;
    bool RemoveMovingPlatformDestinationCell(
        const UGCGeneratedPlatformRegion& sourceRegion,
        const glm::vec3& destinationWorldPosition);
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
    enum class RuntimeActorRefresh {
        KeepCurrentActors,
        ReloadActors,
    };

    bool UpdateMovingPlatformDestinations(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta,
        RuntimeActorRefresh runtimeActorRefresh);
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
