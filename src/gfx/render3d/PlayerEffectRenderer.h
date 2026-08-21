#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Planet;
class Player;
class Renderer3D;

class PlayerEffectRenderer {
public:
    explicit PlayerEffectRenderer(const Renderer3D* renderer);

    void DrawPlayers(const glm::mat4& viewMat) const;
    void DrawEnemyEffects(
        Enemy* enemy,
        const glm::mat4& viewMat,
        const Player* viewportPlayer) const;

private:
    void DrawPlayerCollisionShape(const Player* player) const;
    void DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const;
    void DrawPlayerAttackRange(Player* player) const;
    void DrawEnemyAttackRange(Enemy* enemy) const;
    void DrawEnemyFanAttackRange(Enemy* enemy, float range, float angleRadians) const;
    void DrawFanAttackRange(const Planet* planet, const glm::vec3& center, const glm::vec3& up,
                            const glm::vec3& forward, const glm::vec3& left,
                            float range, float angleRadians, float yOffset) const;
    void DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const;
    void DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const;

private:
    const Renderer3D* mRenderer;
};
