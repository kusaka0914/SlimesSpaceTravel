#include "editor/stage/persistence/StageYamlRepository.h"

#include "utils/AtomicFile.h"
#include "utils/TextFile.h"

#include "actor/Planet.h"
#include "app/Game.h"
#include "world/Stage.h"

#include <algorithm>
#include <filesystem>
#include <utility>

std::string StageYamlRepository::GetCurrentStageYamlPath(DebugEditorContext& context)
{
    if (!context.game) {
        return "";
    }

    return context.game->GetCurrentStageYamlPath();
}

bool StageYamlRepository::LoadCurrentStage(DebugEditorContext& context, YAML::Node& outConfig)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }

    return LoadYamlFile(filePath, outConfig);
}

bool StageYamlRepository::SaveCurrentStage(DebugEditorContext& context, YAML::Node config,
                                           bool preserveRuntimePlanetCenters)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }

    try {
        if (preserveRuntimePlanetCenters && context.game && context.game->GetCurrentStage() && config["planets"] &&
            config["planets"].IsSequence()) {
            const auto& planets = context.game->GetCurrentStage()->GetPlanets();
            const std::size_t count = std::min(planets.size(), config["planets"].size());
            for (std::size_t index = 0; index < count; ++index) {
                const Planet* planet = planets[index];
                if (!planet) {
                    continue;
                }
                const glm::vec3 center = planet->GetPos();
                config["planets"][index]["center"][0] = center.x;
                config["planets"][index]["center"][1] = center.y;
                config["planets"][index]["center"][2] = center.z;
            }
        }

        return SaveYamlFile(filePath, config);
    } catch (const YAML::Exception& error) {
        std::cerr << "Failed to prepare stage yaml: " << filePath << ": " << error.what() << '\n';
        return false;
    }
}

bool StageYamlRepository::ReadCurrentStageText(DebugEditorContext& context, std::string& outYamlText)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }

    std::string error;
    if (!TextFile::Read(filePath, outYamlText, error)) {
        std::cerr << "Failed to read stage yaml: " << filePath << ": " << error << '\n';
        return false;
    }
    return true;
}

bool StageYamlRepository::WriteCurrentStageTextAtomically(DebugEditorContext& context, const std::string& yamlText)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Invalid yaml text. Save cancelled: " << e.what() << std::endl;
        return false;
    }

    std::string errorMessage;
    if (!AtomicFile::WriteText(filePath, yamlText, errorMessage)) {
        std::cerr << "Failed to save stage yaml: " << filePath << ": " << errorMessage << '\n';
        return false;
    }
    return true;
}
