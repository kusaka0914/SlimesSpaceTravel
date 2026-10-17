#pragma once

#include <glm/glm.hpp>
#include <vector>

class NPC;
class Planet;
class Renderer3D;

// NPCの接近メッセージを、ルビ付きのビルボードとして描画する。Renderer3Dは非所有参照。
class NPCProximityMessageRenderer {
public:
    explicit NPCProximityMessageRenderer(const Renderer3D* renderer);

    // 指定惑星の有効なNPCのうち、接近メッセージの表示条件を満たすものだけ描く。
    void Draw(const glm::mat4& viewMat, const std::vector<Planet*>& planets) const;

private:
    void DrawMessage(const glm::mat4& viewMat, const NPC* npc) const;

private:
    const Renderer3D* mRenderer;
};
