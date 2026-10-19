#pragma once

#include <glm/glm.hpp>
#include <string>

class Actor;
class Renderer3D;
class Stage;

// Actorの識別ラベルを3D空間に描画する。Renderer3Dは非所有参照。
class DebugLabelRenderer {
public:
    explicit DebugLabelRenderer(const Renderer3D* renderer);

    // 現在のステージにラベルを描く。UGCモードでは描かない。
    void DrawDebugLabels(const glm::mat4& viewMat, const Stage* stage, bool isUGCMode) const;

private:
    void DrawDebugLabel(const glm::mat4& viewMat, const Actor* actor, const std::string& label) const;

private:
    const Renderer3D* mRenderer;
};
