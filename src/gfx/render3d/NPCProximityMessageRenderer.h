#pragma once

#include <glm/glm.hpp>
#include <vector>

class NPC;
class Planet;
class Renderer3D;

class NPCProximityMessageRenderer {
public:
    explicit NPCProximityMessageRenderer(const Renderer3D* renderer);

    void Draw(
        const glm::mat4& viewMat,
        const std::vector<Planet*>& planets) const;

private:
    void DrawMessage(const glm::mat4& viewMat, const NPC* npc) const;

private:
    const Renderer3D* mRenderer;
};
