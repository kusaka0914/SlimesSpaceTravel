#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageEditorTypes.h"
#include "editor/stage/creation/StageActorNodeFactory.h"
#include "editor/stage/creation/StageActorRuntimeCreationService.h"
#include "editor/stage/history/StageEditOperation.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// Actor追加をYAMLへの保存と実行中ステージへの生成に反映する。配置未指定時は種類ごとの初期配置を使う。
class StageActorCreateService {
public:
    explicit StageActorCreateService(DebugEditorContext& context);

    void SetEditExecutor(StageEditExecutor executeEdit);

    bool AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale,
                     const StageActorPlacement* placement = nullptr);

    // 作成したスイッチの platformId を返す。作成できない場合は std::nullopt。
    std::optional<std::string> AddPressureSwitchPlatform(int currentPlanetNum, const std::string& modelPath,
                                                         const glm::vec3& scale, const std::string& textureOverridePath,
                                                         const std::string& targetPlatformId,
                                                         const StageActorPlacement* placement = nullptr);
    bool AddRideMovingPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale,
                               const StageActorPlacement* placement = nullptr);

    // startPlacement/endPlacement はワールド座標。2点を移動経路の両端として保存する。
    bool AddMovingPlatform(int currentPlanetNum, const StageActorPlacement& startPlacement,
                           const StageActorPlacement& endPlacement, const glm::vec3& scale);
    bool AddFadingPlatform(int currentPlanetNum, const glm::vec3& scale,
                           const StageActorPlacement* placement = nullptr);
    bool AddAdhesivePlatform(int currentPlanetNum, const glm::vec3& scale,
                             const StageActorPlacement* placement = nullptr);
    bool AddTwoPlayerSwitchPair(int currentPlanetNum, const StageActorPlacement& firstPlacement,
                                const StageActorPlacement& secondPlacement, const glm::vec3& scale,
                                const std::string& textureOverridePath, const std::string& targetPlatformId);
    bool AddPlanet(const std::string& modelPath);
    bool AddEllipsePlanet(const std::string& modelPath);
    bool AddEllipsePlanetAtPosition(const std::string& modelPath, const glm::vec3& worldPosition);
    bool AddEnemy(const std::string& type, int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddNPC(const std::string& modelPath, int currentPlanetNum, const std::string& name,
                const std::vector<std::string>& talkTexts, float radius, float scale,
                const StageActorPlacement* placement = nullptr);
    bool AddTutorialTrigger(int currentPlanetNum, const std::string& modelPath,
                            const std::vector<std::string>& talkTexts, const glm::vec3& scale,
                            const StageActorPlacement* placement = nullptr);
    bool AddCrystal(const std::string& type, int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddBoatParts(const std::string& type, int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddBoat(int startPlanetNum, int destPlanetNum, int destStage, const StageActorPlacement* placement = nullptr);
    bool AddBoatArrivalPoint(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale,
                             const StageActorPlacement* placement = nullptr);
    bool AddStar(int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddJewelItem(int currentPlanetNum, const std::string& modelPath, const std::string& texturePath,
                      const glm::vec3& scale, const StageActorPlacement* placement = nullptr);
    bool AddHazardActor(int currentPlanetNum, const std::string& modelPath, const std::string& texturePath,
                        const glm::vec3& scale, float triggerRadius, float damage, float damageIntervalSeconds,
                        const StageActorPlacement* placement = nullptr);
    bool AddCameraZone(int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddStageObject(int currentPlanetNum, const std::string& modelPath, bool collisionEnabled,
                        const StageActorPlacement* placement = nullptr);

    // 複製先の配置に合わせて足場IDと移動経路も調整する。
    // 保存後の生成に失敗した場合は、旧YAMLとRuntimeの復元を試みる。
    bool DuplicateActorAtPlacement(const StageActorRef& sourceRef, const YAML::Node& sourceNode, int targetPlanetIndex,
                                   const StageActorPlacement& placement);

private:
    bool CommitCreation(YAML::Node config, const StageEditOperation& createActors);

    bool CanCreateActor() const;
    bool IsValidPlanetIndex(int planetIndex, const char* label) const;
    void ApplyPlacementToNode(YAML::Node& node, int planetIndex, const StageActorPlacement* placement) const;
    void SynchronizeRuntimeSwitchTargets(const YAML::Node& stageConfig) const;

    void EnsureSequence(YAML::Node& config, const std::string& sequenceName) const;

    DebugEditorContext& mContext;
    StageEditExecutor mExecuteEdit;
    StageActorNodeFactory mNodeFactory;
    StageActorRuntimeCreationService mRuntimeCreationService;
};
