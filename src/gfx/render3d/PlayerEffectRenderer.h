#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Planet;
class Player;
class PhysicsSystem;
class Renderer3D;

class PlayerEffectRenderer {
public:
    explicit PlayerEffectRenderer(const Renderer3D* renderer);

    void DrawPlayers(
        const glm::mat4& viewMat,
        const std::vector<Player*>& players,
        bool isDebugEditorShowing,
        const PhysicsSystem* physicsSystem) const;
    void DrawPlayerMergeGuide(
        const Player* targetPlayer,
        float radiusWorldUnits) const;
    void DrawEnemyEffects(
        Enemy* enemy,
        const glm::mat4& viewMat,
        const Player* viewportPlayer) const;

private:
    void DrawPlayerCollisionShape(
        const Player* player,
        bool isDebugEditorShowing,
        const PhysicsSystem* physicsSystem) const;
    void DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const;
    void DrawPlayerSplitGuard(
        const glm::mat4& viewMat,
        const Player* player,
        int guardCount,
        int maximumGuardCount) const;
    void DrawPlayerAttackRange(Player* player) const;
    void DrawEnemyAttackRange(Enemy* enemy, bool shouldFlashWhite) const;
    void DrawEnemyFanAttackRange(
        Enemy* enemy,
        float range,
        float angleRadians,
        bool shouldFlashWhite) const;
    void DrawFanAttackRange(const Planet* planet, const glm::vec3& center, const glm::vec3& up,
                            const glm::vec3& forward, const glm::vec3& left,
                            float range, float angleRadians, float yOffset,
                            const glm::vec4& fillColor,
                            const glm::vec4& edgeColor) const;
    void DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const;
    void DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const;

private:
    const Renderer3D* mRenderer;
};
