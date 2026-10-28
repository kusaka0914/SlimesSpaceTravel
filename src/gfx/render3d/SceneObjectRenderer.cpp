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

    DrawPlanets(planets);
    mRenderer->DrawEnvironmentDecorations(scene);
    DrawActorOnPlanets(planets, viewMat, viewportPlayer, scene);
    DrawCharacterShadows(planets, shouldDrawPlayers, scene.players);

    if (mPlayerEffectRenderer && shouldDrawPlayers) {
        mPlayerEffectRenderer->DrawPlayers(viewMat, scene.players, scene.isDebugEditorShowing, &mPhysics,
                                           scene.splitGuard, scene.soul);
        if (scene.mergeGuideTarget) {
            mPlayerEffectRenderer->DrawPlayerMergeGuide(scene.mergeGuideTarget, scene.mergeGuideRadiusWorldUnits);
        }
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
                                             const Player* viewportPlayer, const SceneRenderState& scene) const
{
    glUniform1f(mRenderer->GetShader3D()->GetLocToonLevels(), 3.0f);
    glUniform1f(mRenderer->GetShader3D()->GetLocToonStrength(), 0.6f);

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (mRenderer->IsActorInsideView(enemy)) {
                mRenderer->TryDrawActor(enemy, true);
            }
        }

        for (Boat* boat : planet->GetBoats()) {
            if (boat && boat->ShouldRenderUnavailablePreview()) {
                constexpr float unavailableRocketOpacity = 0.2f;
                mRenderer->DrawInactiveActorPreview(boat, unavailableRocketOpacity);
                continue;
            }
            mRenderer->TryDrawActor(boat);
        }
        mRenderer->TryDrawActors(planet->GetBoatParts());
        mRenderer->TryDrawActors(planet->GetCrystals());
        mRenderer->TryDrawActors(planet->GetPlatforms());
        mRenderer->TryDrawActors(planet->GetStageObjects());
        mRenderer->TryDrawActors(planet->GetNPCs());
        mRenderer->TryDrawActors(planet->GetJewelItems());
        mRenderer->TryDrawActors(planet->GetHazardActors());
        if (scene.isDebugEditorShowing) {
            mRenderer->TryDrawActors(planet->GetTutorialTriggers());
        }
        mRenderer->TryDrawActor(planet->GetKey());
        mRenderer->TryDrawActor(planet->GetStar());

        if (scene.isDebugMode) {
            mRenderer->TryDrawActors(planet->GetBoatArrivalPoints());
            mRenderer->TryDrawActors(planet->GetFallRespawnPoints());
        }
    }

    mRenderer->TryDrawActors(scene.runtimeJewels);

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
