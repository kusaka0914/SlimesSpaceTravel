#include "gfx/render3d/SceneObjectRenderer.h"
#include "gfx/Renderer3D.h"

#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "actor/TutorialTrigger.h"
#include "gfx/Shader3D.h"
#include "gfx/render3d/DebugLabelRenderer.h"
#include "gfx/render3d/NPCProximityMessageRenderer.h"
#include "gfx/render3d/PlayerEffectRenderer.h"
#include "world/Stage.h"

#include <GL/glew.h>
#include <algorithm>

SceneObjectRenderer::SceneObjectRenderer(const Renderer3D* renderer, const PhysicsSystem& physics,
                                         const PlayerEffectRenderer* playerEffectRenderer,
                                         const DebugLabelRenderer* debugLabelRenderer,
                                         const NPCProximityMessageRenderer* npcProximityMessageRenderer)
    : mRenderer(renderer),
      mPhysics(physics),
      mPlayerEffectRenderer(playerEffectRenderer),
      mDebugLabelRenderer(debugLabelRenderer),
      mNPCProximityMessageRenderer(npcProximityMessageRenderer)
{
}

void SceneObjectRenderer::DrawSceneObjects(const glm::mat4& viewMat, const Player* viewportPlayer,
                                           const SceneRenderState& scene) const
{
    if (!mRenderer || !mRenderer->GetShader3D() || !scene.stage) {
        return;
    }

    const std::vector<Planet*>& planets = scene.stage->GetPlanets();

    const bool shouldDrawPlayers = !scene.hidesPlayersForCinematic;
    std::vector<TransparentActorDraw> transparentActors;

    DrawPlanets(planets);
    mRenderer->DrawEnvironmentDecorations(scene);
    DrawActorOnPlanets(planets, viewMat, scene, transparentActors);
    DrawCharacterShadows(planets, shouldDrawPlayers, scene.players);

    if (mPlayerEffectRenderer && shouldDrawPlayers) {
        mPlayerEffectRenderer->DrawPlayers(viewMat, scene.players, scene.isDebugEditorShowing, &mPhysics,
                                           scene.splitGuard, scene.soul);
    }

    // 足場やPlayerの深度を確定してから半透明モデルを重ね、手前の不透明物が透けることを防ぐ。
    DrawTransparentActors(transparentActors);
    DrawEnemyEffects(planets, viewMat, viewportPlayer, scene);
    if (mPlayerEffectRenderer && shouldDrawPlayers && scene.mergeGuideTarget) {
        mPlayerEffectRenderer->DrawPlayerMergeGuide(scene.mergeGuideTarget, scene.mergeGuideRadiusWorldUnits);
    }

    if (!scene.isGameUIHidden && mNPCProximityMessageRenderer) {
        mNPCProximityMessageRenderer->Draw(viewMat, planets);
    }

    if (scene.isDebugEditorShowing && mDebugLabelRenderer) {
        mDebugLabelRenderer->DrawDebugLabels(viewMat, scene.stage, scene.isUGCMode);
    }
}

void SceneObjectRenderer::DrawPlanets(const std::vector<Planet*>& planets) const
{
    glUniform1f(mRenderer->GetShader3D()->GetLocToonLevels(), 5.0f);
    glUniform1f(mRenderer->GetShader3D()->GetLocToonStrength(), 0.45f);
    mRenderer->TryDrawActors(planets, false);
}

void SceneObjectRenderer::DrawActorOnPlanets(const std::vector<Planet*>& planets, const glm::mat4& viewMat,
                                             const SceneRenderState& scene,
                                             std::vector<TransparentActorDraw>& transparentActors) const
{
    glUniform1f(mRenderer->GetShader3D()->GetLocToonLevels(), 3.0f);
    glUniform1f(mRenderer->GetShader3D()->GetLocToonStrength(), 0.6f);

    const auto drawActor = [&](Actor* actor, float opacityMultiplier = 1.0f, bool allowsInactive = false) {
        if (!actor || (!actor->GetIsActive() && !allowsInactive)) {
            return;
        }
        const float opacity = glm::clamp(actor->GetRenderOpacity() * opacityMultiplier, 0.0f, 1.0f);
        if (opacity <= 0.001f) {
            return;
        }
        if (opacity < 0.999f) {
            const float depth = (viewMat * glm::vec4(actor->GetRenderPosition(), 1.0f)).z;
            transparentActors.push_back({actor, opacityMultiplier, depth});
            return;
        }
        mRenderer->DrawActor(actor);
    };
    const auto drawActors = [&](const auto& actors) {
        for (auto* actor : actors) {
            drawActor(actor);
        }
    };

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (mRenderer->IsActorInsideView(enemy)) {
                drawActor(enemy);
            }
        }

        for (Boat* boat : planet->GetBoats()) {
            if (boat && boat->ShouldRenderUnavailablePreview()) {
                constexpr float unavailableRocketOpacity = 0.2f;
                drawActor(boat, unavailableRocketOpacity, true);
                continue;
            }
            drawActor(boat);
        }
        drawActors(planet->GetBoatParts());
        drawActors(planet->GetCrystals());
        drawActors(planet->GetPlatforms());
        drawActors(planet->GetStageObjects());
        drawActors(planet->GetNPCs());
        drawActors(planet->GetJewelItems());
        drawActors(planet->GetHazardActors());
        if (scene.isDebugEditorShowing) {
            drawActors(planet->GetTutorialTriggers());
        }
        drawActor(planet->GetKey());
        drawActor(planet->GetStar());

        if (scene.isDebugMode) {
            drawActors(planet->GetBoatArrivalPoints());
            drawActors(planet->GetFallRespawnPoints());
        }
    }

    drawActors(scene.runtimeJewels);
}

void SceneObjectRenderer::DrawTransparentActors(std::vector<TransparentActorDraw>& actors) const
{
    std::stable_sort(actors.begin(), actors.end(),
                     [](const TransparentActorDraw& first, const TransparentActorDraw& second) {
                         return first.viewDepth < second.viewDepth;
                     });
    for (const TransparentActorDraw& draw : actors) {
        mRenderer->DrawInactiveActorPreview(draw.actor, draw.opacityMultiplier);
    }
}

void SceneObjectRenderer::DrawEnemyEffects(const std::vector<Planet*>& planets, const glm::mat4& viewMat,
                                           const Player* viewportPlayer, const SceneRenderState& scene) const
{
    if (!mPlayerEffectRenderer) {
        return;
    }

    const Player* effectReferencePlayer = viewportPlayer ? viewportPlayer : scene.controlledPlayer;
    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            mPlayerEffectRenderer->DrawEnemyEffects(enemy, viewMat, effectReferencePlayer, !scene.isGameUIHidden);
        }
    }
}

void SceneObjectRenderer::DrawCharacterShadows(const std::vector<Planet*>& planets, bool shouldDrawPlayers,
                                               std::span<Player* const> players) const
{
    if (!mRenderer) {
        return;
    }

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }
        for (Enemy* enemy : planet->GetEnemies()) {
            if (!mRenderer->IsActorInsideView(enemy)) {
                continue;
            }
            mRenderer->DrawBlobShadow(enemy);
        }
        for (NPC* npc : planet->GetNPCs()) {
            if (!mRenderer->IsActorInsideView(npc)) {
                continue;
            }
            mRenderer->DrawBlobShadow(npc);
        }
    }

    if (!shouldDrawPlayers) {
        return;
    }

    for (Player* player : players) {
        if (player && player->GetIsActive()) {
            mRenderer->DrawBlobShadow(player);
        }
    }
}
