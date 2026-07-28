#include "gfx/render3d/SceneObjectRenderer.h"
#include "gfx/Renderer3D.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Key.h"
#include "actor/MovingPlatform.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"
#include "actor/StageObject.h"
#include "gfx/Shader3D.h"
#include "gfx/render3d/DebugLabelRenderer.h"
#include "gfx/render3d/NPCProximityMessageRenderer.h"
#include "gfx/render3d/PlayerEffectRenderer.h"

#include <GL/glew.h>

SceneObjectRenderer::SceneObjectRenderer(const Renderer3D* renderer, const PlayerEffectRenderer* playerEffectRenderer,
                                         const DebugLabelRenderer* debugLabelRenderer,
                                         const NPCProximityMessageRenderer* npcProximityMessageRenderer)
    : mRenderer(renderer),
      mPlayerEffectRenderer(playerEffectRenderer),
      mDebugLabelRenderer(debugLabelRenderer),
      mNPCProximityMessageRenderer(npcProximityMessageRenderer)
{
}

void SceneObjectRenderer::DrawSceneObjects(const glm::mat4& viewMat) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetShader3D() || !mRenderer->GetGame()->GetCurrentStage()) {
        return;
    }

    std::vector<Planet*> planets = mRenderer->GetGame()->GetCurrentStage()->GetPlanets();

    DrawPlanets(planets);
    DrawActorOnPlanets(planets, viewMat);

    if (mPlayerEffectRenderer) {
        mPlayerEffectRenderer->DrawPlayers(viewMat);
    }

    if (mNPCProximityMessageRenderer) {
        mNPCProximityMessageRenderer->Draw(viewMat, planets);
    }

    if (mRenderer->GetGame()->GetIsDebugEditorShowing() && mDebugLabelRenderer) {
        mDebugLabelRenderer->DrawDebugLabels(viewMat);
    }
}

void SceneObjectRenderer::DrawPlanets(const std::vector<Planet*>& planets) const
{
    glUniform1f(mRenderer->GetShader3D()->GetLocToonLevels(), 5.0f);
    glUniform1f(mRenderer->GetShader3D()->GetLocToonStrength(), 0.45f);
    mRenderer->TryDrawActors(planets, false);
}

void SceneObjectRenderer::DrawActorOnPlanets(const std::vector<Planet*>& planets, const glm::mat4& viewMat) const
{
    glUniform1f(mRenderer->GetShader3D()->GetLocToonLevels(), 3.0f);
    glUniform1f(mRenderer->GetShader3D()->GetLocToonStrength(), 0.6f);

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        if (mPlayerEffectRenderer) {
            for (Enemy* enemy : planet->GetEnemies()) {
                mPlayerEffectRenderer->DrawEnemyWithEffects(enemy, viewMat);
            }
        }

        mRenderer->TryDrawActors(planet->GetBoats());
        mRenderer->TryDrawActors(planet->GetBoatParts());
        mRenderer->TryDrawActors(planet->GetCrystals());
        mRenderer->TryDrawActors(planet->GetPlatforms());
        mRenderer->TryDrawActors(planet->GetMovingPlatforms());
        mRenderer->TryDrawActors(planet->GetStageObjects());
        mRenderer->TryDrawActors(planet->GetNPCs());
        mRenderer->TryDrawActor(planet->GetKey());
        mRenderer->TryDrawActor(planet->GetStar());

        if (mRenderer->GetGame()->GetIsDebugMode()) {
            mRenderer->TryDrawActors(planet->GetBoatArrivalPoints());
            mRenderer->TryDrawActors(planet->GetFallRespawnPoints());
        }
    }
}
