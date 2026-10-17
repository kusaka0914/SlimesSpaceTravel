#pragma once

#include <glm/glm.hpp>
#include <string>

class Actor;

// ワールド座標の配置位置と、その位置でActorを立てるための地表法線。
struct StageActorPlacement {
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
};

enum class StageActorType {
    Planet,
    Enemy,
    Platform,
    Crystal,
    NPC,
    BoatParts,
    Boat,
    BoatArrivalPoint,
    FallRespawnPoint,
    Key,
    Star,
    StageObject,
    TutorialTrigger,
    JewelItem,
    HazardActor
};

// YAMLシーケンス名と添字でActorを識別する。要素削除・並べ替え後は添字の更新が必要。
struct StageActorRef {
    StageActorType type;
    int yamlIndex = -1;
    std::string sequenceName;
    std::string label;
};

// 実行中ActorとYAML参照の組。Actorは非所有で、ステージ再読み込み後は取得し直す。
struct StageActorInstance {
    Actor* actor = nullptr;
    StageActorRef ref;
};

struct StageActorTypeInfo {
    StageActorType type;
    std::string sequenceName;
    std::string displayName;
};
