#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Player;
class Renderer3D;

class PlayerEffectRenderer {
public:
    explicit PlayerEffectRenderer(const Renderer3D* renderer);

    void DrawPlayers(const glm::mat4& viewMat) const;
    void DrawEnemyWithEffects(Enemy* enemy, const glm::mat4& viewMat) const;

private:
    void DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const;
    void DrawPlayerAttackRange(Player* player) const;
    void DrawEnemyAttackRange(Enemy* enemy) const;
    void DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const;
    void DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const;

private:
    const Renderer3D* mRenderer;
};
