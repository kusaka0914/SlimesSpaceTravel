#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

class Game;
class Player;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game);

    void Draw();

private:
    void DrawPerformance();
    void DrawPlayer();
    void DrawCamera();
    // void DrawStage1();
    void DrawDebugDrawSettings();
    bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);

    template <typename T>
    bool SetYamlSequenceValue(YAML::Node& config, const std::string& sequenceName, std::size_t index,
                              const std::string& key, const T& value)
    {
        if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
            std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
            return false;
        }

        if (index >= config[sequenceName].size()) {
            std::cerr << "Index out of range: " << index << std::endl;
            return false;
        }

        config[sequenceName][index][key] = value;
        return true;
    }

    Game* mGame;
};