#include "editor/stage/StageActorNodeFactory.h"

#include "app/Game.h"
#include "world/Stage.h"
#include "actor/Planet.h"
#include "editor/core/DebugEditorContext.h"

StageActorNodeFactory::StageActorNodeFactory(DebugEditorContext& context)
    : StageActorNodeFactory([&context](int planetIndex, float height) {
          const auto& planets = context.game->GetCurrentStage()->GetPlanets();
          const Planet* planet = planets[planetIndex];
          return planet ? planet->GetRadius() + height : height;
      })
{
}
