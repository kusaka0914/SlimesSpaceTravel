#include "gfx/debug/stage/StageActorNodeFactory.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorContext.h"

StageActorNodeFactory::StageActorNodeFactory(
    DebugEditorContext& context)
    : StageActorNodeFactory(
          [&context](int planetIndex, float height) {
              const auto& planets =
                  context.game->GetCurrentStage()->GetPlanets();
              const Planet* planet = planets[planetIndex];
              return planet ? planet->GetRadius() + height : height;
          })
{
}
