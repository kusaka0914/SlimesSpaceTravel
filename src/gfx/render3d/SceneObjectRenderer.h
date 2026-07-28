#pragma once

#include <glm/glm.hpp>
#include <vector>

class DebugLabelRenderer;
class NPCProximityMessageRenderer;
class Planet;
class PlayerEffectRenderer;
class Renderer3D;

class SceneObjectRenderer {
public:
    SceneObjectRenderer(const Renderer3D* renderer, const PlayerEffectRenderer* playerEffectRenderer,
                        const DebugLabelRenderer* debugLabelRenderer,
                        const NPCProximityMessageRenderer* npcProximityMessageRenderer);

    void DrawSceneObjects(const glm::mat4& viewMat) const;

private:
    void DrawPlanets(const std::vector<Planet*>& planets) const;
    void DrawActorOnPlanets(const std::vector<Planet*>& planets, const glm::mat4& viewMat) const;

private:
    const Renderer3D* mRenderer;
    const PlayerEffectRenderer* mPlayerEffectRenderer;
    const DebugLabelRenderer* mDebugLabelRenderer;
    const NPCProximityMessageRenderer* mNPCProximityMessageRenderer;
};
