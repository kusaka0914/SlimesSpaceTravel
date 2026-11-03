#pragma once

#include "actor/StageObject.h"

#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>

// 領域内のPlayerへ操作画像と説明文を表示する、衝突を持たない回転ボックス。
// Editorでだけ領域を描画し、ゲーム中の表示はUIRendererが担当する。
class OperationGuideZone final : public StageObject {
public:
    static constexpr int MaximumOperations = 3;
    struct Operation {
        std::string iconId = "buttonA";
        std::string description = "ジャンプ";
        bool repeatInput = false;
    };

    explicit OperationGuideZone(Game* game);
    void UpdateActor(float) override {}
    float GetRenderOpacity() const override;
    bool Contains(const glm::vec3& worldPosition) const;

    // 未指定項目は初期値を使用し、操作は最大3件まで読み込む。旧1件形式も読み込める。
    // 保存先はステージ内のguideマップで、操作の順番を保持する。
    void LoadGuide(const YAML::Node& node);
    void SaveGuide(YAML::Node node) const;

    bool enabled = true;
    int priority = 0;
    std::vector<Operation> operations{Operation{}};
};
