#include "editor/stage/creation/StageActorNodeFactory.h"

#include "actor/Planet.h"
#include "app/Game.h"
#include "editor/core/DebugEditorContext.h"
#include "world/Stage.h"

StageActorNodeFactory::StageActorNodeFactory(DebugEditorContext& context)
    : StageActorNodeFactory([&context](int planetIndex, float height) {
          const auto& planets = context.game->GetCurrentStage()->GetPlanets();
          const Planet* planet = planets[planetIndex];
          return planet ? planet->GetRadius() + height : height;
      })
{
}
