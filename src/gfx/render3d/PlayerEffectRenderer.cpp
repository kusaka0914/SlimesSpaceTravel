#include "gfx/render3d/PlayerEffectRenderer.h"

#include "gfx/Renderer3D.h"
#include "gfx/VertexArray.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyAttackGeometry.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/enemy/behavior/EnemyBehaviorAction.h"
#include "gfx/Shader3D.h"
#include "system/PhysicsSystem.h"
#include "system/physics/EllipsoidCollisionShapeGeometry.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <initializer_list>

namespace {
const glm::vec4 attackWarningFillColor(1.0f, 0.1f, 0.1f, 0.18f);
const glm::vec4 attackWarningEdgeColor(1.0f, 0.1f, 0.1f, 0.75f);
const glm::vec4 attackImpactFillColor(1.0f, 1.0f, 1.0f, 0.62f);
const glm::vec4 attackImpactEdgeColor(1.0f);
const glm::vec4 mergeGuideFillColor(0.2f, 0.9f, 1.0f, 0.16f);
const glm::vec4 mergeGuideEdgeColor(0.35f, 1.0f, 0.75f, 0.9f);

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
        return modelTop + 0.25f;
    }
    return enemy.GetRadius() * 0.8f;
}
}

PlayerEffectRenderer::PlayerEffectRenderer(const Renderer3D* renderer)
    : mRenderer(renderer)
{
}

void PlayerEffectRenderer::DrawPlayers(
    const glm::mat4& viewMat,
    const std::vector<Player*>& players,
    bool isDebugEditorShowing,
    const PhysicsSystem* physicsSystem) const
{
    if (!mRenderer) {
        return;
    }

    if (players.empty() || !players[0]) {
        return;
    }

    mRenderer->TryDrawActor(players[0]);
    DrawPlayerCollisionShape(
        players[0], isDebugEditorShowing, physicsSystem);
    DrawTiredEffect(viewMat, players[0]);

    const bool hasPlayer2 = players.size() >= 2 && players[1];
    if (!hasPlayer2) {
        return;
    }

    mRenderer->TryDrawActor(players[1]);
    DrawPlayerCollisionShape(
        players[1], isDebugEditorShowing, physicsSystem);
    DrawTiredEffect(viewMat, players[1]);
}

void PlayerEffectRenderer::DrawPlayerMergeGuide(
    const Player* targetPlayer,
    float radiusWorldUnits) const
{
    if (!mRenderer ||
        !targetPlayer ||
        !targetPlayer->GetIsActive() ||
        radiusWorldUnits <= 0.0f) {
        return;
    }

    const Platform* groundPlatform = nullptr;
    if (targetPlayer->GetOnGround()) {
        groundPlatform = dynamic_cast<const Platform*>(
            targetPlayer->GetGroundActor());
    }

    glm::vec3 up = targetPlayer->GetUpVec();
    if (groundPlatform) {
        up = groundPlatform->GetUpVec();
    }
    if (glm::dot(up, up) <= 0.000001f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up = glm::normalize(up);
    }

    glm::vec3 forward = targetPlayer->GetFacingForwardVec();
    forward -= up * glm::dot(forward, up);
    if (glm::dot(forward, forward) <= 0.000001f) {
        glm::vec3 fallbackAxis(1.0f, 0.0f, 0.0f);
        if (std::abs(up.y) < 0.9f) {
            fallbackAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        forward = glm::cross(fallbackAxis, up);
    }
    forward = glm::normalize(forward);
    const glm::vec3 left = glm::normalize(glm::cross(up, forward));

    constexpr float surfaceOffsetWorldUnits = 0.06f;
    const glm::vec3 center = targetPlayer->GetPos();
    const Planet* planet = targetPlayer->GetCurrentPlanet();
    const bool shouldFollowPlanetSurface =
        !groundPlatform &&
        planet &&
        planet->GetPlanetShape() != Planet::PlanetShape::Normal;
    float playerSurfaceOffset = 0.0f;
    if (shouldFollowPlanetSurface) {
        const Planet::EllipseSurfaceProjection centerProjection =
            planet->CalculateEllipseSurfaceProjection(center);
        playerSurfaceOffset = glm::dot(
            center - centerProjection.position,
            centerProjection.outwardNormal);
    }

    const auto resolveSurfacePoint =
        [planet,
         shouldFollowPlanetSurface,
         playerSurfaceOffset,
         surfaceOffsetWorldUnits,
         up](const glm::vec3& point) {
            if (!shouldFollowPlanetSurface) {
                return point + up * surfaceOffsetWorldUnits;
            }
            const Planet::EllipseSurfaceProjection projection =
                planet->CalculateEllipseSurfaceProjection(point);
            return projection.position +
                   projection.outwardNormal *
                       (playerSurfaceOffset + surfaceOffsetWorldUnits);
        };

    constexpr int segmentCount = 72;
    std::vector<glm::vec3> fillVertices;
    fillVertices.reserve(segmentCount + 2);
    fillVertices.push_back(resolveSurfacePoint(center));
    for (int segmentIndex = 0;
         segmentIndex <= segmentCount;
         ++segmentIndex) {
        const float angle =
            glm::two_pi<float>() *
            static_cast<float>(segmentIndex) /
            static_cast<float>(segmentCount);
        const glm::vec3 direction =
            forward * std::cos(angle) + left * std::sin(angle);
        fillVertices.push_back(resolveSurfacePoint(
            center + direction * radiusWorldUnits));
    }
    mRenderer->DrawAttackRangeVertices(
        fillVertices, GL_TRIANGLE_FAN, mergeGuideFillColor);

    constexpr float edgeThicknessWorldUnits = 0.08f;
    const float innerRadius = std::max(
        0.0f,
        radiusWorldUnits - edgeThicknessWorldUnits);
    std::vector<glm::vec3> edgeVertices;
    edgeVertices.reserve((segmentCount + 1) * 2);
    for (int segmentIndex = 0;
         segmentIndex <= segmentCount;
         ++segmentIndex) {
        const float angle =
            glm::two_pi<float>() *
            static_cast<float>(segmentIndex) /
            static_cast<float>(segmentCount);
        const glm::vec3 direction =
            forward * std::cos(angle) + left * std::sin(angle);
        edgeVertices.push_back(resolveSurfacePoint(
            center + direction * radiusWorldUnits));
        edgeVertices.push_back(resolveSurfacePoint(
            center + direction * innerRadius));
    }
    mRenderer->DrawAttackRangeVertices(
        edgeVertices, GL_TRIANGLE_STRIP, mergeGuideEdgeColor);
}

void PlayerEffectRenderer::DrawPlayerCollisionShape(
    const Player* player,
    bool isDebugEditorShowing,
    const PhysicsSystem* physicsSystem) const
{
    if (!mRenderer || !player || !player->GetIsActive() ||
        !isDebugEditorShowing || !physicsSystem) {
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

    const Player* controlledPlayer = viewportPlayer;
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

    const bool shouldDrawAttackWarning =
        enemy->ShouldDrawAttackPreview() &&
        enemy->GetStandByAttackTimer() > 0.0f &&
        enemy->GetStandByAttackTimer() <= 1.0f;
    const bool shouldDrawAttackImpactFlash =
        enemy->ShouldDrawAttackImpactFlash();
    if (enemy->IsAlive() &&
        enemy->IsOnGround() &&
        (shouldDrawAttackWarning || shouldDrawAttackImpactFlash)) {
        EnemyAttackPreview preview;
        if (enemy->GetBehaviorAttackPreview(preview) &&
            (preview.shape == EnemyAttackPreviewShape::Fan ||
             preview.shape == EnemyAttackPreviewShape::Radial)) {
            DrawEnemyFanAttackRange(
                enemy,
                preview.range,
                preview.angleRadians,
                shouldDrawAttackImpactFlash);
        } else {
            DrawEnemyAttackRange(enemy, shouldDrawAttackImpactFlash);
        }
    }
}

void PlayerEffectRenderer::DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const
{
    if (!mRenderer || !mRenderer->GetShader3D()) {
        return;
    }

    if (!player || !player->GetIsActive() || !player->GetIsTired()) {
        return;
    }

    const GLuint tiredStarTexture =
        mRenderer->FindTexture("tired_star");
    if (tiredStarTexture == 0) {
        return;
    }

    VertexArray* quad = mRenderer->FindVertexArray("quad");
    if (!quad) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tiredStarTexture);
    glUniform1i(shader->GetLocDiffuseTexture(), 0);
    glUniform1i(shader->GetLocUseTexture(), 1);
    glUniform4f(shader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    quad->SetActive();

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

        const glm::mat4 billboard = mRenderer->CreateBillboard(
            viewMat, center + orbitOffset, up, scale, scale);

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
        0.06f,
        attackWarningFillColor,
        attackWarningEdgeColor);
}

void PlayerEffectRenderer::DrawEnemyFanAttackRange(
    Enemy* enemy,
    float range,
    float angleRadians,
    bool shouldFlashWhite) const
{
    if (!mRenderer || !enemy) {
        return;
    }

    EnemyAttackFrame attackFrame = ResolveEnemyAttackFrame(*enemy);
    attackFrame.origin +=
        attackFrame.forward *
        CalculateEnemyAttackFrontOffset(*enemy, attackFrame);
    const glm::vec4& fillColor = shouldFlashWhite
        ? attackImpactFillColor
        : attackWarningFillColor;
    const glm::vec4& edgeColor = shouldFlashWhite
        ? attackImpactEdgeColor
        : attackWarningEdgeColor;
    DrawFanAttackRange(
        enemy->GetCurrentPlanet(),
        attackFrame.origin,
        attackFrame.up,
        attackFrame.forward,
        attackFrame.left,
        range,
        angleRadians,
        0.56f,
        fillColor,
        edgeColor);
}

void PlayerEffectRenderer::DrawFanAttackRange(
    const Planet* planet, const glm::vec3& center, const glm::vec3& up, const glm::vec3& forward,
    const glm::vec3& left, float range, float angleRadians, float yOffset,
    const glm::vec4& fillColor,
    const glm::vec4& edgeColor) const
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

    mRenderer->DrawAttackRangeVertices(
        fanVertices, GL_TRIANGLE_FAN, fillColor);

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

    mRenderer->DrawAttackRangeVertices(
        edgeVertices, GL_TRIANGLE_STRIP, edgeColor);
}

void PlayerEffectRenderer::DrawEnemyAttackRange(
    Enemy* enemy,
    bool shouldFlashWhite) const
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
        CalculateEnemyMeleeAttackPreviewArea(*enemy, attackFrame);
    if (previewArea.forwardLength <= 0.0f ||
        previewArea.halfWidth <= 0.0f) {
        return;
    }

    constexpr float yOffset = 0.56f;
    constexpr float thickness = 0.08f;
    const glm::vec4& fillColor = shouldFlashWhite
        ? attackImpactFillColor
        : attackWarningFillColor;
    const glm::vec4& edgeColor = shouldFlashWhite
        ? attackImpactEdgeColor
        : attackWarningEdgeColor;
    const glm::vec3 start =
        attackFrame.origin +
        attackFrame.forward * previewArea.forwardStartOffset +
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
        fillColor);

    const std::vector<glm::vec3> leftEdgeVertices{
        start + attackFrame.left * previewArea.halfWidth,
        start + attackFrame.left * innerHalfWidth,
        end + attackFrame.left * previewArea.halfWidth,
        end + attackFrame.left * innerHalfWidth};
    mRenderer->DrawAttackRangeVertices(
        leftEdgeVertices,
        GL_TRIANGLE_STRIP,
        edgeColor);

    const std::vector<glm::vec3> rightEdgeVertices{
        start - attackFrame.left * previewArea.halfWidth,
        start - attackFrame.left * innerHalfWidth,
        end - attackFrame.left * previewArea.halfWidth,
        end - attackFrame.left * innerHalfWidth};
    mRenderer->DrawAttackRangeVertices(
        rightEdgeVertices,
        GL_TRIANGLE_STRIP,
        edgeColor);

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
        edgeColor);

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
        edgeColor);
}

void PlayerEffectRenderer::DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const
{
    if (!mRenderer || !mRenderer->GetShader3D() || !enemy ||
        !enemy->IsAlive()) {
        return;
    }

    const int breakCount = enemy->GetBreakCount();
    if (breakCount == 0) {
        return;
    }

    const GLuint guardTexture = mRenderer->FindTexture("guard");
    VertexArray* quad = mRenderer->FindVertexArray("quad");
    if (guardTexture == 0 || !quad) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, guardTexture);
    glUniform1i(shader->GetLocUseTexture(), 1);
    quad->SetActive();

    const float upMargin = CalculateEnemyGuardHeight(*enemy);
    constexpr float guardWidth = 0.5f;
    constexpr float guardHeight = 0.5f;

    for (int i = 0; i < breakCount; i++) {
        const float rightMargin = (i - (breakCount - 1) * 0.5f) * 0.4f;
        glm::mat4 billboard =
            mRenderer->CreateBillboard(
                viewMat, enemy, upMargin, rightMargin, guardWidth, guardHeight);
        glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1i(shader->GetLocUseTexture(), 0);
    mRenderer->EndTransparentDraw();
}

void PlayerEffectRenderer::DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const
{
    if (!mRenderer || !mRenderer->GetShader3D() || !enemy) {
        return;
    }

    VertexArray* hpBar = mRenderer->FindVertexArray("hpBar");
    if (!hpBar) {
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();
    hpBar->SetActive();

    constexpr float rightMargin = -0.5f;


    const float upMargin = CalculateEnemyGuardHeight(*enemy) + 0.40f;
    const float hpWidth = enemy->GetHp() / enemy->GetMaxHp();
    constexpr float hpHeight = 0.1f;

    const glm::mat4 billboard = mRenderer->CreateBillboard(
        viewMat, enemy, upMargin, rightMargin, hpWidth, hpHeight);
    glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));

    std::vector<GLfloat> hpGreen{0.0f, 1.0f, 0.0f, 1.0f};
    glUniform4fv(shader->GetLocObjectColor(), 1, hpGreen.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    mRenderer->EndTransparentDraw();
}
