#pragma once

#include "effect/particle/ParticleTypes.h"
#include "system/camera/CameraProjection.h"

#include <glm/glm.hpp>
#include <optional>
#include <span>

class Actor;
class JewelItem;
class Player;
class Stage;

struct SplitGuardRenderState {
    const Player* controlledPlayer = nullptr;
    int count = 0;
    int maximumCount = 0;
};

// 描画呼出中だけ有効なシーン情報。Actorと一覧の所有権は持たず、次の更新・再読込をまたいで保持しない。
// 一覧はspanで借用し、フレームごとの複製・割当を行わない。
struct SceneRenderState {
    const Stage* stage = nullptr;
    int stageNumber = 0;
    std::span<Player* const> players{};
    std::span<JewelItem* const> runtimeJewels{};
    std::span<const Particle> particles{};
    Player* controlledPlayer = nullptr;

    bool isTitle = false;
    bool isBattleStyleSelection = false;
    bool hidesPlayersForCinematic = false;
    bool isUGCMode = false;
    bool isDebugEditorShowing = false;
    bool isDebugMode = false;
    bool isGameUIHidden = false;
    bool isPlayer2Joined = false;
    CameraProjectionSettings projection{};
    SplitGuardRenderState splitGuard{};
    const Player* mergeGuideTarget = nullptr;
    float mergeGuideRadiusWorldUnits = 0.0f;

    int ugcEditLayer = 0;
    float ugcGridSize = 1.0f;
    std::optional<glm::vec3> placementPosition{};
    Actor* placementModel = nullptr;
    std::span<const glm::vec3> placementModelPositions{};
    std::optional<glm::vec3> movingPathStart{};
    std::optional<glm::vec3> movingPathDestination{};
};
