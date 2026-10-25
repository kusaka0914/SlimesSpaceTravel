#include "editor/stage/persistence/StagePlanetYamlWriter.h"

#include "actor/Actor.h"
#include "actor/Planet.h"
#include "app/Game.h"
#include "world/Stage.h"

#include <glm/glm.hpp>
#include <string>

StagePlanetYamlWriter::StagePlanetYamlWriter(DebugEditorContext& context)
    : mContext(context)
{
}

bool StagePlanetYamlWriter::WriteSettings(YAML::Node& config) const
{
    return Write(config, false);
}

bool StagePlanetYamlWriter::WriteEditorAuthoredTransforms(YAML::Node& config) const
{
    return Write(config, true);
}

bool StagePlanetYamlWriter::Write(YAML::Node& config, bool shouldSaveEditorTransform) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }
    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    for (std::size_t planetIndex = 0; planetIndex < planets.size(); ++planetIndex) {
        Planet* planet = planets[planetIndex];
        if (!planet) {
            continue;
        }

        const EditorAuthoredTransform* editorTransform =
            shouldSaveEditorTransform ? planet->FindEditorAuthoredTransform() : nullptr;
        if (editorTransform && editorTransform->hasPosition) {
            const glm::vec3& center = editorTransform->localPosition;
            config["planets"][planetIndex]["center"][0] = center.x;
            config["planets"][planetIndex]["center"][1] = center.y;
            config["planets"][planetIndex]["center"][2] = center.z;
        }
        if (editorTransform && editorTransform->hasScale) {
            const glm::vec3& scale = editorTransform->scale;
            config["planets"][planetIndex]["scale"][0] = scale.x;
            config["planets"][planetIndex]["scale"][1] = scale.y;
            config["planets"][planetIndex]["scale"][2] = scale.z;
        }

        if (shouldSaveEditorTransform) {
            continue;
        }

        YAML::Node planetNode = config["planets"][planetIndex];
        planetNode.remove("shape");
        planetNode["model"] = planet->GetModelPath();

        const std::string& textureOverride = planet->GetTextureOverridePath();
        if (textureOverride.empty()) {
            planetNode.remove("textureOverride");
        } else {
            planetNode["textureOverride"] = textureOverride;
        }

        const glm::vec2 textureTiling = planet->GetTextureTiling();
        planetNode["textureTiling"][0] = textureTiling.x;
        planetNode["textureTiling"][1] = textureTiling.y;

        const std::string& backTextureOverride = planet->GetBackTextureOverridePath();
        if (backTextureOverride.empty()) {
            planetNode.remove("backTextureOverride");
        } else {
            planetNode["backTextureOverride"] = backTextureOverride;
        }
        planetNode["textureSideBlendWidth"] = planet->GetTextureSideBlendWidth();
        planetNode["canAttractNearbyPlayer"] = planet->CanAttractNearbyPlayer();
        planetNode["reactsToOverheadGravityRay"] = planet->ShouldReactToOverheadGravityRay();
        planetNode["rocketSpawnCondition"] = planet->GetRocketSpawnCondition();
    }

    return true;
}
