#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

void Require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void ValidateTree(const YAML::Node& node, std::vector<YAML::Node>& ancestors)
{
    Require(ancestors.size() < 128, "configuration nesting exceeds 128 levels");
    for (const auto& ancestor : ancestors) {
        Require(!node.is(ancestor), "cyclic YAML alias");
    }
    ancestors.push_back(node);
    if (node.IsMap()) {
        std::set<std::string> keys;
        for (const auto& entry : node) {
            Require(entry.first.IsScalar(), "mapping key must be scalar");
            const std::string key = entry.first.as<std::string>();
            Require(keys.insert(key).second, "duplicate mapping key: " + key);
            ValidateTree(entry.second, ancestors);
        }
    } else if (node.IsSequence()) {
        for (const auto& child : node) {
            ValidateTree(child, ancestors);
        }
    }
    ancestors.pop_back();
}

void ValidateVector(const YAML::Node& value, const std::string& field, std::size_t size)
{
    if (!value || value.IsNull()) {
        return;
    }
    // scaleのスカラーは既存の設定形式として許容する。
    if (field == "scale" && value.IsScalar()) {
        Require(std::isfinite(value.as<double>()), "non-finite scale");
        return;
    }
    Require(value.IsSequence() && value.size() == size, "invalid vector dimension: " + field);
    for (const auto& component : value) {
        Require(std::isfinite(component.as<double>()), "non-finite vector: " + field);
    }
}

void ValidateStage(const YAML::Node& root)
{
    const YAML::Node planets = root["planets"];
    if (!planets) {
        return;
    }
    Require(planets.IsSequence(), "planets must be a sequence");
    Require(planets.size() > 0, "stage must contain at least one planet");
    const std::set<std::string> actorGroups{"planets",
                                            "players",
                                            "platforms",
                                            "movingPlatforms",
                                            "enemies",
                                            "star",
                                            "keys",
                                            "boatParts",
                                            "boats",
                                            "NPCs",
                                            "crystals",
                                            "stageObjects",
                                            "jewelItems",
                                            "hazardActors",
                                            "boatArrivalPoints",
                                            "fallRespawnPoints",
                                            "tutorialTriggers",
                                            "ugcPlatformCells"};
    std::set<std::string> platformIds;
    for (const auto& entry : root) {
        const auto& sequence = entry.second;
        const std::string group = entry.first.as<std::string>();
        if (!actorGroups.contains(group)) {
            continue;
        }
        Require(sequence.IsSequence(), "actor/cell group must be a sequence: " + group);
        for (std::size_t index = 0; index < sequence.size(); ++index) {
            const auto actor = sequence[index];
            Require(actor.IsMap(), "actor/cell must be a mapping: " + group);
            for (const auto* field : {"center", "pos", "localPos", "upVec", "rotation", "scale"}) {
                ValidateVector(actor[field], field, 3);
            }
            ValidateVector(actor["rotationQuat"], "rotationQuat", 4);
            for (const auto* field : {"currentPlanetNum", "planetIndex"}) {
                if (actor[field]) {
                    const int planetIndex = actor[field].as<int>();
                    Require(planetIndex < 0 || static_cast<std::size_t>(planetIndex) < planets.size(),
                            "missing planet reference: " + std::string(field));
                }
            }
            if (group == "platforms" || group == "movingPlatforms") {
                const auto id = actor["platformId"].as<std::string>("legacy_" + group + "_" + std::to_string(index));
                Require(!id.empty() && platformIds.insert(id).second, "duplicate/empty platform ID: " + id);
            }
        }
    }
    for (const auto& entry : root) {
        if (!entry.second.IsSequence()) {
            continue;
        }
        for (const auto& actor : entry.second) {
            if (!actor.IsMap()) {
                continue;
            }
            const auto components = actor["components"];
            if (!components || !components.IsMap()) {
                continue;
            }
            const auto pressure = components["pressureSwitch"];
            if (pressure && pressure.IsMap() && pressure["targets"]) {
                Require(pressure["targets"].IsSequence(), "pressure switch targets must be a sequence");
                for (const auto& target : pressure["targets"]) {
                    const auto id = target.as<std::string>();
                    Require(platformIds.contains(id), "missing platform target: " + id);
                }
            }
        }
    }
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        std::cerr << "Usage: game_asset_check <YAML file or directory>\n";
        return 2;
    }
    try {
        const std::filesystem::path input(argv[1]);
        std::vector<std::filesystem::path> files;
        if (std::filesystem::is_regular_file(input)) {
            files.push_back(input);
        } else {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) {
                if (entry.is_regular_file() &&
                    (entry.path().extension() == ".yaml" || entry.path().extension() == ".yml")) {
                    files.push_back(entry.path());
                }
            }
        }
        Require(!files.empty(), "no YAML files found");
        std::sort(files.begin(), files.end());
        int failures = 0;
        for (const auto& file : files) {
            try {
                // filesystem::pathで開き、Windowsでも日本語ファイル名を狭い文字列へ変換しない。
                std::ifstream inputFile(file);
                Require(static_cast<bool>(inputFile), "cannot open YAML file");
                const auto documents = YAML::LoadAll(inputFile);
                Require(documents.size() == 1 && documents.front().IsMap(), "expected one mapping document");
                std::vector<YAML::Node> ancestors;
                ValidateTree(documents.front(), ancestors);
                ValidateStage(documents.front());
            } catch (const std::exception& error) {
                ++failures;
                const auto name = file.u8string();
                std::cerr << std::string(name.begin(), name.end()) << ": " << error.what() << '\n';
            }
        }
        std::cout << "Checked " << files.size() << " YAML files; " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
