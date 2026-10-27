#pragma once

#include "editor/stage/StageEditorTypes.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

class Stage;

enum class StageHierarchyCategory {
    Enemy,
    Platform,
    Item,
    NPC,
    Boat,
    CameraZone,
    TutorialTrigger,
    Hazard,
    StageObject,
    Other,
};

struct StageHierarchyActorGroup {
    StageHierarchyCategory category = StageHierarchyCategory::Other;
    std::string label;
    std::vector<StageActorInstance> actors;
    std::size_t actorCount = 0;
};

struct StageHierarchyPlanetGroup {
    // 惑星自身は件数に含めない。未所属グループではplanet.actorがnullptr、planetIndexが-1になる。
    StageActorInstance planet{nullptr, {StageActorType::Planet, -1, "planets", ""}};
    int planetIndex = -1;
    std::string label;
    std::vector<StageHierarchyActorGroup> groups;
    std::size_t actorCount = 0;
};

// Runtimeの所属惑星と既存のYAML参照から、一覧表示専用の分類を構築する。Actorは所有しない。
class StageActorHierarchy {
public:
    // 空の惑星も残し、種類は空なら省く。所属惑星を特定できないActorは未所属に含める。stageがnullptrなら空配列。
    // 返却データは再読み込み前までの借用参照を含むため、描画ごとに収集し直す。
    static std::vector<StageHierarchyPlanetGroup> Collect(Stage* stage);

    // 名前・種類・惑星名を部分一致で絞る。英字の大小と空白を区別せず、元の件数・順序・分類は保つ。
    static std::vector<StageHierarchyPlanetGroup> Filter(const std::vector<StageHierarchyPlanetGroup>& hierarchy,
                                                         std::string_view query);
};
