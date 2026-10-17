#pragma once

#include "gfx/debug/stage/StageActorNodeFactory.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/stage/UGCPlatformDocument.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct DebugEditorContext;

// UGCセルの変更から生成足場を再構築し、YAML保存とステージへの反映を行う。
class UGCPlatformCellService {
public:
    explicit UGCPlatformCellService(DebugEditorContext& context);

    // セルを追加して生成足場を再構築・保存し、ステージを再読み込みする。失敗時は false。
    bool AddCell(int planetIndex, const StageActorPlacement& placement, float gridSize, int footprintSideLength = 1,
                 const std::string& behavior = "normal", const glm::ivec3& movementDeltaCells = glm::ivec3(0));
    bool AddCells(int planetIndex, const std::vector<glm::ivec3>& anchorGridPositions, float gridSize,
                  int footprintSideLength, const std::string& behavior,
                  const glm::ivec3& movementDeltaCells = glm::ivec3(0));
    bool RefreshGeneratedPlatforms();

    // 生成足場に対応するセルをworldDeltaから求めたグリッド差分だけ移動し、保存・再読み込みする。
    bool TranslateCells(const StageActorRef& generatedPlatformRef, const glm::vec3& worldDelta);
    bool TranslateCells(const std::vector<StageActorRef>& generatedPlatformRefs, const glm::vec3& worldDelta);

    // 移動先だけをグリッド単位で変更し、生成足場を再構築・保存・再読み込みする。
    bool TranslateMovingPlatformDestinations(const std::vector<StageActorRef>& generatedPlatformRefs,
                                             const glm::vec3& worldDelta);

    // 移動先の変更を再構築・保存するが、操作中のActorを保つためステージを再読み込みしない。
    bool SaveMovingPlatformDestinationTranslation(const std::vector<StageActorRef>& generatedPlatformRefs,
                                                  const glm::vec3& worldDelta);
    bool RemoveMovingPlatformDestinationCell(const StageActorRef& generatedPlatformRef,
                                             const glm::vec3& destinationWorldPosition);
    bool ResolveMovingPlatformRegion(const StageActorRef& generatedPlatformRef,
                                     UGCGeneratedPlatformRegion& outRegion) const;
    bool RemoveMovingPlatformDestinationCell(const UGCGeneratedPlatformRegion& sourceRegion,
                                             const glm::vec3& destinationWorldPosition);
    bool RemoveCellAtGridPosition(int planetIndex, const glm::vec3& worldPosition, float gridSize, int gridLayer);

    // 同じX/Z列の指定レイヤーを優先し、なければ最上段を返す。セルがない場合は false。
    bool ResolveLayerAtGridPosition(int planetIndex, const glm::vec3& worldPosition, float gridSize,
                                    int preferredGridLayer, int& outGridLayer) const;

    // 同じX/Z列の最上段の1つ上を返す。空の列では emptyColumnGridLayer を返す。
    int ResolvePlacementLayerAtGridPosition(int planetIndex, const glm::vec3& worldPosition, float gridSize,
                                            int emptyColumnGridLayer) const;

private:
    enum class RuntimeActorRefresh {
        KeepCurrentActors,
        ReloadActors,
    };

    bool UpdateMovingPlatformDestinations(const std::vector<StageActorRef>& generatedPlatformRefs,
                                          const glm::vec3& worldDelta, RuntimeActorRefresh runtimeActorRefresh);
    bool CanEditCells() const;
    bool IsValidPlanetIndex(int planetIndex) const;
    bool RebuildGeneratedPlatforms(YAML::Node& stageConfig) const;
    void ApplyPlacement(YAML::Node& actorNode, int planetIndex, const StageActorPlacement& placement) const;

    DebugEditorContext& mContext;
    StageActorNodeFactory mNodeFactory;
};
