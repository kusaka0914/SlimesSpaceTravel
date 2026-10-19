#pragma once

#include <glm/glm.hpp>
#include <span>
#include <vector>

class DebugLabelRenderer;
class NPCProximityMessageRenderer;
class Planet;
class Player;
class PlayerEffectRenderer;
class Renderer3D;
class PhysicsSystem;
struct SceneRenderState;

// 惑星・Actor・影・補助表示の描画順を組み立てる。渡されたRenderer群はすべて非所有参照。
class SceneObjectRenderer {
public:
    SceneObjectRenderer(const Renderer3D* renderer, const PhysicsSystem& physics,
                        const PlayerEffectRenderer* playerEffectRenderer, const DebugLabelRenderer* debugLabelRenderer,
                        const NPCProximityMessageRenderer* npcProximityMessageRenderer);

    // 現在のステージを描く。viewportPlayerは敵の状態表示の基準で、nullptrなら操作中のPlayerを使う。
    void DrawSceneObjects(const glm::mat4& viewMat, const Player* viewportPlayer, const SceneRenderState& scene) const;

private:
    void DrawPlanets(const std::vector<Planet*>& planets) const;
    void DrawCharacterShadows(const std::vector<Planet*>& planets, bool shouldDrawPlayers,
                              std::span<Player* const> players) const;
    void DrawActorOnPlanets(const std::vector<Planet*>& planets, const glm::mat4& viewMat, const Player* viewportPlayer,
                            const SceneRenderState& scene) const;

private:
    const Renderer3D* mRenderer;
    const PhysicsSystem& mPhysics;
    const PlayerEffectRenderer* mPlayerEffectRenderer;
    const DebugLabelRenderer* mDebugLabelRenderer;
    const NPCProximityMessageRenderer* mNPCProximityMessageRenderer;
};
