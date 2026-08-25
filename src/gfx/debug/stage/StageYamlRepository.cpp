#include "gfx/debug/stage/StageYamlRepository.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

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

    try {
        outConfig = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

bool StageYamlRepository::SaveCurrentStage(
    DebugEditorContext& context,
    YAML::Node config,
    bool preserveRuntimePlanetCenters)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }





    if (preserveRuntimePlanetCenters &&
        context.game && context.game->GetCurrentStage() &&
        config["planets"] && config["planets"].IsSequence()) {
        const auto& planets = context.game->GetCurrentStage()->GetPlanets();
        const std::size_t count = std::min(planets.size(), config["planets"].size());
        for (std::size_t index = 0; index < count; ++index) {
            const Planet* planet = planets[index];
            if (!planet) continue;
            const glm::vec3 center = planet->GetPos();
            config["planets"][index]["center"][0] = center.x;
            config["planets"][index]["center"][1] = center.y;
            config["planets"][index]["center"][2] = center.z;
        }
    }

    return SaveYamlFile(filePath, config);
}

bool StageYamlRepository::ReadCurrentStageText(DebugEditorContext& context, std::string& outYamlText)
{
    const std::string filePath = GetCurrentStageYamlPath(context);

    if (filePath.empty()) {
        std::cerr << "Current stage yaml path is empty" << std::endl;
        return false;
    }

    std::ifstream ifs(filePath);

    if (!ifs) {
        std::cerr << "Failed to open stage yaml: " << filePath << std::endl;
        return false;
    }

    outYamlText.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

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

    const std::string tempPath = filePath + ".tmp";

    {
        std::ofstream ofs(tempPath, std::ios::out | std::ios::trunc);

        if (!ofs) {
            std::cerr << "Failed to open temp yaml: " << tempPath << std::endl;
            return false;
        }

        ofs << yamlText;
        ofs.close();

        if (!ofs) {
            std::cerr << "Failed to write temp yaml completely: " << tempPath << std::endl;
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, filePath, ec);

    if (ec) {
        std::error_code removeEc;

        if (std::filesystem::exists(filePath, removeEc)) {
            std::filesystem::remove(filePath, removeEc);
        }

        if (removeEc) {
            std::cerr << "Failed to remove old yaml: " << filePath << std::endl;
            std::cerr << removeEc.message() << std::endl;
            return false;
        }

        ec.clear();
        std::filesystem::rename(tempPath, filePath, ec);

        if (ec) {
            std::cerr << "Failed to rename temp yaml: " << tempPath << " -> " << filePath << std::endl;
            std::cerr << ec.message() << std::endl;
            return false;
        }
    }

    return true;
}

bool StageYamlRepository::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

bool StageYamlRepository::RemoveSequenceElement(YAML::Node& config, const std::string& sequenceName, int index)
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
        return false;
    }

    YAML::Node oldSeq = config[sequenceName];

    if (index < 0 || index >= static_cast<int>(oldSeq.size())) {
        std::cerr << "Delete index out of range: " << index << std::endl;
        return false;
    }

    YAML::Node newSeq(YAML::NodeType::Sequence);

    for (int i = 0; i < static_cast<int>(oldSeq.size()); ++i) {
        if (i == index) {
            continue;
        }

        newSeq.push_back(oldSeq[i]);
    }

    config[sequenceName] = newSeq;
    return true;
}
