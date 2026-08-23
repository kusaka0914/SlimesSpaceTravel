#include "gfx/render3d/PlayerEffectRenderer.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "gfx/VertexArray.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyAttackGeometry.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/enemy/behavior/EnemyBehaviorAction.h"
#include "gfx/Shader3D.h"
#include "system/PhysicsSystem.h"
#include "system/physics/EllipsoidCollisionShapeGeometry.h"
#include "utils/MathUtils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <initializer_list>

namespace {
bool ShouldFollowSphereSurface(const Planet* planet)
{
    return planet && planet->GetPlanetShape() ==
                         Planet::PlanetShape::Sphere;
}

glm::vec3 ProjectOntoSphereSurface(
    const Planet* planet,
    float surfaceRadius,
    const glm::vec3& position)
{
    if (!ShouldFollowSphereSurface(planet)) {
        return position;
    }

    const glm::vec3 fromCenter = position - planet->GetPos();
    if (glm::dot(fromCenter, fromCenter) < 0.000001f) {
        return position;
    }
    return planet->GetPos() +
           glm::normalize(fromCenter) * surfaceRadius;
}

float CalculateEnemyGuardHeight(const Enemy& enemy)
{
    glm::vec3 up = enemy.GetUpVec();
    if (glm::dot(up, up) <= 0.000001f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up = glm::normalize(up);
    }

    EnemyCollisionGeometry::ModelBounds modelBounds;
    if (EnemyCollisionGeometry::TryCreateModelBounds(enemy, modelBounds)) {
        const float modelTop =
            glm::dot(modelBounds.center - enemy.GetPos(), up) +
            EnemyCollisionGeometry::CalculateSupportDistance(modelBounds, up);
        // The guard sits just above the actual model, independent of the
        // model's scale or its local origin.
        return modelTop + 0.25f;
    }

    // Models without mesh bounds retain the previous safe fallback.
    return enemy.GetRadius() * 0.8f;
}
} // namespace

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
    DrawPlayerCollisionShape(players[0]);

    /*const bool canDrawP1AttackRange =
        players[0]->IsAttacking() || players[0]->GetIsStrongAttacked() || players[0]->GetCanSpecialAttack();
    if (canDrawP1AttackRange) {
        DrawPlayerAttackRange(players[0]);
    }*/

    DrawTiredEffect(viewMat, players[0]);

    const bool hasPlayer2 = players.size() >= 2 && players[1];
    if (!hasPlayer2) {
        return;
    }

    mRenderer->TryDrawActor(players[1]);
    DrawPlayerCollisionShape(players[1]);

    /*const bool canDrawP2AttackRange =
        players[1]->IsAttacking() || players[1]->GetIsStrongAttacked() || players[1]->GetCanSpecialAttack();
    if (canDrawP2AttackRange) {
        DrawPlayerAttackRange(players[1]);
    }*/

    DrawTiredEffect(viewMat, players[1]);
}

void PlayerEffectRenderer::DrawPlayerCollisionShape(
    const Player* player) const
{
    if (!mRenderer || !player || !player->GetIsActive()) {
        return;
    }

    Game* game = mRenderer->GetGame();
    if (!game || !game->GetIsDebugEditorShowing()) {
        return;
    }

    const PhysicsSystem* physicsSystem = game->GetPhysicsSystem();
    if (!physicsSystem) {
        return;
    }

    const float collisionScaleMultiplier =
        player->GetCollisionScaleMultiplier();
    const float collisionWidth =
        physicsSystem->GetPlayerCollisionWidth() *
        collisionScaleMultiplier;
    const float collisionHeight =
        physicsSystem->GetPlayerCollisionHeight() *
        collisionScaleMultiplier;
    const float collisionDepth =
        physicsSystem->GetPlayerCollisionDepth() *
        collisionScaleMultiplier;
    if (collisionWidth <= 0.0f ||
        collisionHeight <= 0.0f ||
        collisionDepth <= 0.0f) {
        return;
    }

    glm::quat collisionOrientation = player->GetOrientation();
    if (glm::length(collisionOrientation) < 1e-6f) {
        collisionOrientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
    collisionOrientation = glm::normalize(collisionOrientation);

    const glm::vec3 collisionCenter =
        player->GetPos() +
        player->GetUpVec() *
            physicsSystem->GetPlayerCollisionCenterHeight() *
            collisionScaleMultiplier;
    const glm::vec4 collisionColor(0.1f, 0.9f, 1.0f, 0.9f);

    const auto calculateWorldSurfacePoint =
        [collisionWidth,
         collisionHeight,
         collisionDepth,
         collisionCenter,
         collisionOrientation](int latitudeIndex, int longitudeIndex) {
            const glm::vec3 localSurfacePoint =
                EllipsoidCollisionShapeGeometry::CalculateLocalSurfacePoint(
                    collisionWidth,
                    collisionHeight,
                    collisionDepth,
                    latitudeIndex,
                    longitudeIndex);
            return collisionCenter + collisionOrientation * localSurfacePoint;
        };

    std::vector<glm::vec3> wireframeVertices;
    const int latitudeSegmentCount =
        EllipsoidCollisionShapeGeometry::LatitudeSegmentCount;
    const int longitudeSegmentCount =
        EllipsoidCollisionShapeGeometry::LongitudeSegmentCount;
    const int latitudeEdgeCount =
        (latitudeSegmentCount - 1) * longitudeSegmentCount;
    const int longitudeEdgeCount =
        latitudeSegmentCount * longitudeSegmentCount;
    wireframeVertices.reserve(
        static_cast<std::size_t>(
            (latitudeEdgeCount + longitudeEdgeCount) * 2));

    for (int latitudeIndex = 1;
         latitudeIndex < latitudeSegmentCount;
         ++latitudeIndex) {
        for (int longitudeIndex = 0;
             longitudeIndex < longitudeSegmentCount;
             ++longitudeIndex) {
            const int nextLongitudeIndex =
                (longitudeIndex + 1) % longitudeSegmentCount;
            wireframeVertices.push_back(
                calculateWorldSurfacePoint(
                    latitudeIndex,
                    longitudeIndex));
            wireframeVertices.push_back(
                calculateWorldSurfacePoint(
                    latitudeIndex,
                    nextLongitudeIndex));
        }
    }

    for (int longitudeIndex = 0;
         longitudeIndex < longitudeSegmentCount;
         ++longitudeIndex) {
        for (int latitudeIndex = 0;
             latitudeIndex < latitudeSegmentCount;
             ++latitudeIndex) {
            wireframeVertices.push_back(
                calculateWorldSurfacePoint(
                    latitudeIndex,
                    longitudeIndex));
            wireframeVertices.push_back(
                calculateWorldSurfacePoint(
                    latitudeIndex + 1,
                    longitudeIndex));
        }
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLfloat previousLineWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    mRenderer->DrawAttackRangeVertices(
        wireframeVertices,
        GL_LINES,
        collisionColor);

    glLineWidth(previousLineWidth);
    if (wasDepthTestEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
}

void PlayerEffectRenderer::DrawEnemyEffects(
    Enemy* enemy,
    const glm::mat4& viewMat,
    const Player* viewportPlayer) const
{
    if (!mRenderer ||
        !enemy ||
        !enemy->GetIsActive() ||
        !mRenderer->IsActorInsideView(enemy)) {
        return;
    }

    Game* game = mRenderer->GetGame();
    const Player* controlledPlayer = viewportPlayer
        ? viewportPlayer
        : (game ? game->GetControlledPlayer() : nullptr);
    const Planet* playerPlanet =
        controlledPlayer
            ? controlledPlayer->GetCurrentPlanet()
            : nullptr;
    const bool isEnemyOnPlayerPlanet =
        playerPlanet &&
        enemy->GetCurrentPlanet() == playerPlanet;
    if (isEnemyOnPlayerPlanet) {
        DrawEnemyGuard(viewMat, enemy);
        DrawEnemyHp(viewMat, enemy);
    }

    if (enemy->IsAlive() &&
        enemy->ShouldDrawAttackPreview() &&
        enemy->GetStandByAttackTimer() > 0.0f &&
        enemy->GetStandByAttackTimer() <= 1.0f) {
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
        player->GetCurrentPlanet(),
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

    const EnemyAttackFrame attackFrame =
        ResolveEnemyAttackFrame(*enemy);
    DrawFanAttackRange(
        enemy->GetCurrentPlanet(),
        attackFrame.origin,
        attackFrame.up,
        attackFrame.forward,
        attackFrame.left,
        range,
        angleRadians,
        0.56f);
}

void PlayerEffectRenderer::DrawFanAttackRange(
    const Planet* planet, const glm::vec3& center, const glm::vec3& up, const glm::vec3& forward,
    const glm::vec3& left, float range, float angleRadians, float yOffset) const
{
    if (!mRenderer || range <= 0.0f || angleRadians <= 0.0f) {
        return;
    }

    constexpr int segments = 48;
    const float halfAngle = angleRadians * 0.5f;
    const float surfaceRadius =
        ShouldFollowSphereSurface(planet)
            ? glm::length(center + up * yOffset - planet->GetPos())
            : 0.0f;
    const auto surfacePoint = [planet, surfaceRadius](
                                  const glm::vec3& point) {
        return ProjectOntoSphereSurface(planet, surfaceRadius, point);
    };

    std::vector<glm::vec3> fanVertices;
    fanVertices.reserve(segments + 2);

    fanVertices.emplace_back(surfacePoint(center + up * yOffset));

    for (int i = 0; i <= segments; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = glm::mix(-halfAngle, halfAngle, t);

        glm::vec3 dir = glm::normalize(forward * std::cos(angle) + left * std::sin(angle));
        fanVertices.emplace_back(
            surfacePoint(center + dir * range + up * yOffset));
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
        edgeVertices.emplace_back(surfacePoint(
            center + dir * outerRadius + up * yOffset));
        edgeVertices.emplace_back(surfacePoint(
            center + dir * innerRadius + up * yOffset));
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

    const EnemyAttackFrame attackFrame =
        ResolveEnemyAttackFrame(*enemy);
    const EnemyMeleeAttackPreviewArea previewArea =
        CalculateEnemyMeleeAttackPreviewArea(*enemy);
    if (previewArea.forwardLength <= 0.0f ||
        previewArea.halfWidth <= 0.0f) {
        return;
    }

    constexpr float yOffset = 0.56f;
    constexpr float thickness = 0.08f;
    const glm::vec3 start =
        attackFrame.origin +
        attackFrame.up * yOffset;
    const glm::vec3 end =
        start +
        attackFrame.forward *
            previewArea.forwardLength;
    const float innerHalfWidth =
        std::max(
            0.0f,
            previewArea.halfWidth - thickness);

    const Planet* planet = enemy->GetCurrentPlanet();
    if (ShouldFollowSphereSurface(planet)) {
        // A large planar rectangle is especially misleading on a sphere.
        // Tessellate it, then lift every sample back onto the planet surface.
        const float surfaceRadius = glm::length(start - planet->GetPos());
        const auto surfacePoint = [planet, surfaceRadius](
                                      const glm::vec3& point) {
            return ProjectOntoSphereSurface(planet, surfaceRadius, point);
        };
        const auto pointAt = [&start,
                              &end,
                              &attackFrame,
                              &surfacePoint](float lengthT,
                                             float sideways) {
            return surfacePoint(
                glm::mix(start, end, lengthT) +
                attackFrame.left * sideways);
        };
        constexpr int lengthSegments = 24;
        constexpr int widthSegments = 8;
        const glm::vec4 fillColor(1.0f, 0.1f, 0.1f, 0.18f);
        const glm::vec4 edgeColor(1.0f, 0.1f, 0.1f, 0.75f);

        for (int lengthIndex = 0;
             lengthIndex < lengthSegments;
             ++lengthIndex) {
            const float fromT = static_cast<float>(lengthIndex) /
                                static_cast<float>(lengthSegments);
            const float toT = static_cast<float>(lengthIndex + 1) /
                              static_cast<float>(lengthSegments);
            std::vector<glm::vec3> strip;
            strip.reserve((widthSegments + 1) * 2);
            for (int widthIndex = 0;
                 widthIndex <= widthSegments;
                 ++widthIndex) {
                const float widthT = static_cast<float>(widthIndex) /
                                     static_cast<float>(widthSegments);
                const float sideways = glm::mix(
                    -previewArea.halfWidth,
                    previewArea.halfWidth,
                    widthT);
                strip.emplace_back(pointAt(fromT, sideways));
                strip.emplace_back(pointAt(toT, sideways));
            }
            mRenderer->DrawAttackRangeVertices(
                strip, GL_TRIANGLE_STRIP, fillColor);
        }

        const auto drawSideEdge = [&pointAt, &edgeColor, this](
                                     float outerSide,
                                     float innerSide) {
            std::vector<glm::vec3> strip;
            strip.reserve((lengthSegments + 1) * 2);
            for (int index = 0; index <= lengthSegments; ++index) {
                const float lengthT = static_cast<float>(index) /
                                      static_cast<float>(lengthSegments);
                strip.emplace_back(pointAt(lengthT, outerSide));
                strip.emplace_back(pointAt(lengthT, innerSide));
            }
            mRenderer->DrawAttackRangeVertices(
                strip, GL_TRIANGLE_STRIP, edgeColor);
        };
        drawSideEdge(previewArea.halfWidth, innerHalfWidth);
        drawSideEdge(-previewArea.halfWidth, -innerHalfWidth);

        const auto drawEndEdge = [&pointAt,
                                  &edgeColor,
                                  &previewArea,
                                  this](float lengthT) {
            std::vector<glm::vec3> strip;
            strip.reserve((widthSegments + 1) * 2);
            const float insetLengthT =
                lengthT <= 0.0f ? 0.01f : 0.99f;
            for (int index = 0; index <= widthSegments; ++index) {
                const float widthT = static_cast<float>(index) /
                                     static_cast<float>(widthSegments);
                const float sideways = glm::mix(
                    -previewArea.halfWidth,
                    previewArea.halfWidth,
                    widthT);
                strip.emplace_back(pointAt(lengthT, sideways));
                strip.emplace_back(pointAt(insetLengthT, sideways));
            }
            mRenderer->DrawAttackRangeVertices(
                strip, GL_TRIANGLE_STRIP, edgeColor);
        };
        drawEndEdge(0.0f);
        drawEndEdge(1.0f);
        return;
    }

    const std::vector<glm::vec3> fillVertices{
        start + attackFrame.left * previewArea.halfWidth,
        start - attackFrame.left * previewArea.halfWidth,
        end + attackFrame.left * previewArea.halfWidth,
        end - attackFrame.left * previewArea.halfWidth};
    mRenderer->DrawAttackRangeVertices(
        fillVertices,
        GL_TRIANGLE_STRIP,
        glm::vec4(1.0f, 0.1f, 0.1f, 0.18f));

    const std::vector<glm::vec3> leftEdgeVertices{
        start + attackFrame.left * previewArea.halfWidth,
        start + attackFrame.left * innerHalfWidth,
        end + attackFrame.left * previewArea.halfWidth,
        end + attackFrame.left * innerHalfWidth};
    mRenderer->DrawAttackRangeVertices(
        leftEdgeVertices,
        GL_TRIANGLE_STRIP,
        glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    const std::vector<glm::vec3> rightEdgeVertices{
        start - attackFrame.left * previewArea.halfWidth,
        start - attackFrame.left * innerHalfWidth,
        end - attackFrame.left * previewArea.halfWidth,
        end - attackFrame.left * innerHalfWidth};
    mRenderer->DrawAttackRangeVertices(
        rightEdgeVertices,
        GL_TRIANGLE_STRIP,
        glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    const glm::vec3 innerStart =
        start + attackFrame.forward * thickness;
    const std::vector<glm::vec3> startEdgeVertices{
        start + attackFrame.left * previewArea.halfWidth,
        start - attackFrame.left * previewArea.halfWidth,
        innerStart + attackFrame.left * previewArea.halfWidth,
        innerStart - attackFrame.left * previewArea.halfWidth};
    mRenderer->DrawAttackRangeVertices(
        startEdgeVertices,
        GL_TRIANGLE_STRIP,
        glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    const glm::vec3 innerEnd =
        end - attackFrame.forward * thickness;
    const std::vector<glm::vec3> endEdgeVertices{
        end + attackFrame.left * previewArea.halfWidth,
        innerEnd + attackFrame.left * previewArea.halfWidth,
        end - attackFrame.left * previewArea.halfWidth,
        innerEnd - attackFrame.left * previewArea.halfWidth};
    mRenderer->DrawAttackRangeVertices(
        endEdgeVertices,
        GL_TRIANGLE_STRIP,
        glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));
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

    const float upMargin = CalculateEnemyGuardHeight(*enemy);
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
    // Keep the health bar a fixed small distance above the guard row, using
    // the model's actual head height rather than the collision radius.
    const float upMargin = CalculateEnemyGuardHeight(*enemy) + 0.40f;
    const float hpWidth = enemy->GetHp() / enemy->GetMaxHp();
    constexpr float hpHeight = 0.1f;

    glm::mat4 billboard = mRenderer->GetGame()->GetMathUtils()->CreateBillBoard(viewMat, enemy, upMargin, rightMargin, hpWidth, hpHeight);
    glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));

    std::vector<GLfloat> hpGreen{0.0f, 1.0f, 0.0f, 1.0f};
    glUniform4fv(shader->GetLocObjectColor(), 1, hpGreen.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    mRenderer->EndTransparentDraw();
}
