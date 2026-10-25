#pragma once

#include <glm/glm.hpp>
#include <span>
#include <vector>

class Enemy;
class Planet;
class Player;
class PhysicsSystem;
class Renderer3D;
struct SplitGuardRenderState;

// Player本体と補助表示、Enemyの攻撃予告・状態表示を描画する。Renderer3Dは非所有参照。
class PlayerEffectRenderer {
public:
    explicit PlayerEffectRenderer(const Renderer3D* renderer);

    // Player本体・疲労・分裂ガードを描き、エディター表示中は衝突形状も重ねる。
    void DrawPlayers(const glm::mat4& viewMat, std::span<Player* const> players, bool isDebugEditorShowing,
                     const PhysicsSystem* physicsSystem, const SplitGuardRenderState& splitGuard) const;

    // 合体可能な範囲をワールド単位の半径で描く。地形に応じて床面または惑星表面に沿わせる。
    void DrawPlayerMergeGuide(const Player* targetPlayer, float radiusWorldUnits) const;

    // 攻撃予告を描く。状態UIはviewportPlayerと同じ惑星の敵に表示し、nullptrやshowsStatusUIがfalseなら状態UIだけを省く。
    void DrawEnemyEffects(Enemy* enemy, const glm::mat4& viewMat, const Player* viewportPlayer,
                          bool showsStatusUI) const;

private:
    void DrawPlayerCollisionShape(const Player* player, bool isDebugEditorShowing,
                                  const PhysicsSystem* physicsSystem) const;
    void DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const;
    void DrawPlayerSplitGuard(const glm::mat4& viewMat, const Player* player, int guardCount, int maximumGuardCount,
                              const PhysicsSystem* physicsSystem) const;
    void DrawEnemyAttackRange(Enemy* enemy, bool shouldFlashWhite) const;
    void DrawEnemyFanAttackRange(Enemy* enemy, float range, float angleRadians, bool shouldFlashWhite) const;
    void DrawFanAttackRange(const Planet* planet, const glm::vec3& center, const glm::vec3& up,
                            const glm::vec3& forward, const glm::vec3& left, float range, float angleRadians,
                            float yOffset, const glm::vec4& fillColor, const glm::vec4& edgeColor) const;
    void DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const;
    void DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const;

private:
    const Renderer3D* mRenderer;
};
