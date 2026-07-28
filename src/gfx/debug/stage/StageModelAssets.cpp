#include "gfx/debug/stage/StageModelAssets.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
std::string ToLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

bool IsSupportedModelExtension(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    return extension == ".obj" || extension == ".fbx" || extension == ".gltf" ||
           extension == ".glb" || extension == ".dae";
}
}

std::vector<std::string> StageModelAssets::Collect()
{
    std::vector<std::string> modelPaths;
    const std::filesystem::path modelDirectory("../assets/models");
    std::error_code error;

    for (std::filesystem::recursive_directory_iterator it(modelDirectory, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error) || !IsSupportedModelExtension(it->path())) {
            continue;
        }

        const std::filesystem::path relativePath =
            std::filesystem::relative(it->path(), modelDirectory, error);
        if (error) {
            error.clear();
            continue;
        }

        modelPaths.emplace_back(relativePath.generic_string());
    }

    std::sort(modelPaths.begin(), modelPaths.end());
    return modelPaths;
}
