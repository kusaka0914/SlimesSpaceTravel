#pragma once

#include "editor/stage/StageEditorTypes.h"

#include <cstddef>
#include <vector>
#include <yaml-cpp/yaml.h>

// identityは重複除去だけに使う非所有キー。sourceIndexは元のヒット配列の添字。
struct StagePickCandidate {
    const void* identity = nullptr;
    std::size_t sourceIndex = 0;
    int layer = 0;
};

namespace StagePickPolicy {

// 有効な生成床のYAMLレイヤーを優先し、未定義・不正なメタデータはワールドY座標へフォールバックする。
int ResolveLayer(const YAML::Node& stage, const StageActorRef& actor, float worldY, float gridSize);

// 入力順を保って重複を除く。通常は編集レイヤー、該当なしなら最上段、全レイヤー指定時は全候補を返す。
std::vector<std::size_t> SelectLayers(const std::vector<StagePickCandidate>& candidates, int editLayer,
                                      bool includeAllLayers);

} // namespace StagePickPolicy
