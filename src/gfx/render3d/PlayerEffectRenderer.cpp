#include "gfx/render3d/PlayerEffectRenderer.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "gfx/VertexArray.h"
#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/behavior/EnemyBehaviorAction.h"
#include "gfx/Shader3D.h"
#include "utils/MathUtils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

PlayerEffectRenderer::PlayerEffectRenderer(const Renderer3D* renderer)
    : mRenderer(renderer)
{
}

void PlayerEffectRenderer::DrawPlayers(const glm::mat4& viewMat) const
{
    if (!mRenderer || !mRenderer->GetGame()) {
        return;
    }

    const std::vector<Player*>& players = mRenderer->GetGame()->GetPlayers();
    if (players.empty() || !players[0]) {
        return;
    }

    mRenderer->TryDrawActor(players[0]);

    /*const bool canDrawP1AttackRange =
        players[0]->IsAttacking() || players[0]->GetIsStrongAttacked() || players[0]->GetCanSpecialAttack();
    if (canDrawP1AttackRange) {
        DrawPlayerAttackRange(players[0]);
    }*/

    DrawTiredEffect(viewMat, players[0]);

    const bool hasPlayer2 = mRenderer->GetGame()->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if (!hasPlayer2) {
        return;
    }

    mRenderer->TryDrawActor(players[1]);

    /*const bool canDrawP2AttackRange =
        players[1]->IsAttacking() || players[1]->GetIsStrongAttacked() || players[1]->GetCanSpecialAttack();
    if (canDrawP2AttackRange) {
        DrawPlayerAttackRange(players[1]);
    }*/

    DrawTiredEffect(viewMat, players[1]);
}

void PlayerEffectRenderer::DrawEnemyWithEffects(Enemy* enemy, const glm::mat4& viewMat) const
{
    if (!mRenderer || !enemy || !enemy->GetIsActive()) {
        return;
    }

    mRenderer->DrawActor(enemy, true);
    DrawEnemyGuard(viewMat, enemy);
    DrawEnemyHp(viewMat, enemy);

    if (enemy->GetStandByAttackTimer() > 0.0f && enemy->GetStandByAttackTimer() <= 1.0f) {
        EnemyAttackPreview preview;
        if (enemy->GetBehaviorAttackPreview(preview) &&
            (preview.shape == EnemyAttackPreviewShape::Fan ||
             preview.shape == EnemyAttackPreviewShape::Radial)) {
            DrawEnemyFanAttackRange(enemy, preview.range, preview.angleRadians);
        } else {
            DrawEnemyAttackRange(enemy);
        }
    }
}

void PlayerEffectRenderer::DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetShader3D()) {
        return;
    }

    if (!player || !player->GetIsActive() || !player->GetIsTired()) {
        return;
    }

    auto& textures = const_cast<Renderer3D*>(mRenderer)->GetTextures();
    auto texIt = textures.find("tired_star");
    if (texIt == textures.end()) {
        return;
    }

    auto& vertexArrays = const_cast<Renderer3D*>(mRenderer)->GetVertexArrays();
    auto quadIt = vertexArrays.find("quad");
    if (quadIt == vertexArrays.end()) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texIt->second);
    glUniform1i(shader->GetLocDiffuseTexture(), 0);
    glUniform1i(shader->GetLocUseTexture(), 1);
    glUniform4f(shader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    quadIt->second->SetActive();

    constexpr int starCount = 3;
    constexpr float orbitRadius = 0.35f;
    constexpr float headHeight = 1.35f;
    constexpr float starSize = 0.28f;
    constexpr float rotateSpeed = 4.5f;

    const float time = static_cast<float>(glfwGetTime());

    glm::vec3 up = player->GetUpVec();
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = player->GetFacingForwardVec();
    forward = forward - up * glm::dot(forward, up);

    if (glm::length(forward) < 1e-6f) {
        forward = player->GetForwardVec();
        forward = forward - up * glm::dot(forward, up);
    }

    if (glm::length(forward) < 1e-6f) {
        forward = glm::vec3(0.0f, 0.0f, 1.0f);
        forward = forward - up * glm::dot(forward, up);
    }

    forward = glm::normalize(forward);

    glm::vec3 left = glm::normalize(glm::cross(up, forward));
    glm::vec3 center = player->GetPos() + up * headHeight;

    for (int i = 0; i < starCount; ++i) {
        const float angle =
            time * rotateSpeed + glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(starCount);

        const glm::vec3 orbitOffset = forward * std::cos(angle) * orbitRadius + left * std::sin(angle) * orbitRadius;
        const float scale = starSize * (1.0f + 0.15f * std::sin(time * 8.0f + static_cast<float>(i)));

        glm::mat4 billboard = mRenderer->GetGame()->GetMathUtils()->CreateBillBoard(viewMat, center + orbitOffset, up, scale, scale);

        glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1i(shader->GetLocUseTexture(), 0);
    mRenderer->EndTransparentDraw();
}

void PlayerEffectRenderer::DrawPlayerAttackRange(Player* player) const
{
    if (!mRenderer || !player) {
        return;
    }

    DrawFanAttackRange(
        player->GetPos(),
        glm::normalize(player->GetUpVec()),
        glm::normalize(player->GetFacingForwardVec()),
        glm::normalize(player->GetLeftVec()),
        player->GetAttackRange(),
        player->GetAttackAngle(),
        0.06f);
}

void PlayerEffectRenderer::DrawEnemyFanAttackRange(
    Enemy* enemy, float range, float angleRadians) const
{
    if (!mRenderer || !enemy) {
        return;
    }

    DrawFanAttackRange(
        enemy->GetPos(),
        glm::normalize(enemy->GetUpVec()),
        glm::normalize(enemy->GetFacingForwardVec()),
        glm::normalize(enemy->GetLeftVec()),
        range,
        angleRadians,
        0.56f);
}

void PlayerEffectRenderer::DrawFanAttackRange(
    const glm::vec3& center, const glm::vec3& up, const glm::vec3& forward,
    const glm::vec3& left, float range, float angleRadians, float yOffset) const
{
    if (!mRenderer || range <= 0.0f || angleRadians <= 0.0f) {
        return;
    }

    constexpr int segments = 48;
    const float halfAngle = angleRadians * 0.5f;

    std::vector<glm::vec3> fanVertices;
    fanVertices.reserve(segments + 2);

    fanVertices.emplace_back(center + up * yOffset);

    for (int i = 0; i <= segments; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = glm::mix(-halfAngle, halfAngle, t);

        glm::vec3 dir = glm::normalize(forward * std::cos(angle) + left * std::sin(angle));
        fanVertices.emplace_back(center + dir * range + up * yOffset);
    }

    mRenderer->DrawAttackRangeVertices(fanVertices, GL_TRIANGLE_FAN, glm::vec4(1.0f, 0.1f, 0.1f, 0.18f));

    constexpr float thickness = 0.08f;
    const float innerRadius = std::max(0.0f, range - thickness);
    const float outerRadius = range;

    std::vector<glm::vec3> edgeVertices;
    edgeVertices.reserve((segments + 1) * 2);

    for (int i = 0; i <= segments; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = glm::mix(-halfAngle, halfAngle, t);

        glm::vec3 dir = glm::normalize(forward * std::cos(angle) + left * std::sin(angle));
        edgeVertices.emplace_back(center + dir * outerRadius + up * yOffset);
        edgeVertices.emplace_back(center + dir * innerRadius + up * yOffset);
    }

    mRenderer->DrawAttackRangeVertices(edgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));
}

void PlayerEffectRenderer::DrawEnemyAttackRange(Enemy* enemy) const
{
    if (!mRenderer || !enemy) {
        return;
    }

    const float enemyAttackRange = enemy->GetAttackRange();

    if (enemyAttackRange <= 0.0f) {
        return;
    }

    const glm::vec3 center = enemy->GetPos();
    const glm::vec3 up = enemy->GetUpVec();
    const glm::vec3 forward = glm::normalize(enemy->GetFacingForwardVec());
    const glm::vec3 left = glm::normalize(enemy->GetLeftVec());
    const float enemyRadius = enemy->GetRadius() * enemy->GetScale().x;
    const glm::vec3 start = center + forward * enemy->GetRadius();
    const glm::vec3 end = start + forward * enemyAttackRange;

    std::vector<glm::vec3> fanVertices;
    fanVertices.reserve(4);

    constexpr float yOffset = 0.56f;

    fanVertices.emplace_back(start + left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(start + -left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(end + -left * enemyRadius + up * yOffset);

    mRenderer->DrawAttackRangeVertices(fanVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.18f));

    std::vector<glm::vec3> leftEdgeVertices;
    constexpr float thickness = 0.08f;

    leftEdgeVertices.emplace_back(start + left * enemyRadius + up * yOffset);
    leftEdgeVertices.emplace_back(start + left * (enemyRadius - thickness) + up * yOffset);
    leftEdgeVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    leftEdgeVertices.emplace_back(end + left * (enemyRadius - thickness) + up * yOffset);

    mRenderer->DrawAttackRangeVertices(leftEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    std::vector<glm::vec3> rightEdgeVertices;

    rightEdgeVertices.emplace_back(start - left * enemyRadius + up * yOffset);
    rightEdgeVertices.emplace_back(start - left * (enemyRadius - thickness) + up * yOffset);
    rightEdgeVertices.emplace_back(end - left * enemyRadius + up * yOffset);
    rightEdgeVertices.emplace_back(end - left * (enemyRadius - thickness) + up * yOffset);

    mRenderer->DrawAttackRangeVertices(rightEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    std::vector<glm::vec3> frontEdgeVertices;

    frontEdgeVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - forward * thickness + left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - forward * thickness - left * enemyRadius + up * yOffset);

    mRenderer->DrawAttackRangeVertices(frontEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));
}

void PlayerEffectRenderer::DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetShader3D() || !enemy ||
        !enemy->IsAlive()) {
        return;
    }

    const int breakCount = enemy->GetBreakCount();
    if (breakCount == 0) {
        return;
    }

    auto& textures = const_cast<Renderer3D*>(mRenderer)->GetTextures();
    auto& vertexArrays = const_cast<Renderer3D*>(mRenderer)->GetVertexArrays();
    auto guardIt = textures.find("guard");
    auto quadIt = vertexArrays.find("quad");
    if (guardIt == textures.end() || quadIt == vertexArrays.end()) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, guardIt->second);
    glUniform1i(shader->GetLocUseTexture(), 1);
    quadIt->second->SetActive();

    const float upMargin = enemy->GetRadius() * 0.8f;
    constexpr float guardWidth = 0.5f;
    constexpr float guardHeight = 0.5f;

    for (int i = 0; i < breakCount; i++) {
        const float rightMargin = (i - (breakCount - 1) * 0.5f) * 0.4f;
        glm::mat4 billboard =
            mRenderer->GetGame()->GetMathUtils()->CreateBillBoard(viewMat, enemy, upMargin, rightMargin, guardWidth, guardHeight);
        glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1i(shader->GetLocUseTexture(), 0);
    mRenderer->EndTransparentDraw();
}

void PlayerEffectRenderer::DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetShader3D() || !enemy) {
        return;
    }

    auto& vertexArrays = const_cast<Renderer3D*>(mRenderer)->GetVertexArrays();
    auto hpBarIt = vertexArrays.find("hpBar");
    if (hpBarIt == vertexArrays.end()) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();
    hpBarIt->second->SetActive();

    constexpr float rightMargin = -0.5f;
    const float upMargin = enemy->GetRadius() * 1.5f;
    const float hpWidth = enemy->GetHp() / enemy->GetMaxHp();
    constexpr float hpHeight = 0.1f;

    glm::mat4 billboard = mRenderer->GetGame()->GetMathUtils()->CreateBillBoard(viewMat, enemy, upMargin, rightMargin, hpWidth, hpHeight);
    glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));

    std::vector<GLfloat> hpGreen{0.0f, 1.0f, 0.0f, 1.0f};
    glUniform4fv(shader->GetLocObjectColor(), 1, hpGreen.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    mRenderer->EndTransparentDraw();
}
