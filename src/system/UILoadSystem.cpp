#include "UILoadSystem.h"
#include <fstream>
#include <iostream>

UILoadSystem::UILoadSystem()
{
    Initialize();
}

void UILoadSystem::Initialize()
{
    LoadUIInfo("../assets/data/ui/ui.yaml");
}

void UILoadSystem::LoadUIInfo(const std::string& path)
{
    const YAML::Node root = YAML::LoadFile(path);

    for (auto screen : root) {
        const std::string screenName = screen.first.as<std::string>();
        const YAML::Node elements = screen.second;

        for (auto node : elements) {
            const std::string type = node["type"] ? node["type"].as<std::string>() : "";

            if (type == "texture") {
                LoadTextureInfo(screenName, node);
            } else if (type == "text") {
                LoadTextInfo(screenName, node);
            }
        }
    }
}

void UILoadSystem::LoadTextureInfo(const std::string& screenName, YAML::Node& node)
{
    TextureInfo info;
    info.x = node["pos"][0] ? node["pos"][0].as<float>() : 0.0f;
    info.y = node["pos"][1] ? node["pos"][1].as<float>() : 0.0f;
    info.xRatio = node["posRatio"][0] ? node["posRatio"][0].as<float>() : 0.0f;
    info.yRatio = node["posRatio"][1] ? node["posRatio"][1].as<float>() : 0.0f;
    info.width = node["scale"][0] ? node["scale"][0].as<float>() : 0.0f;
    info.height = node["scale"][1] ? node["scale"][1].as<float>() : 0.0f;
    info.widthRatio = node["scaleRatio"][0] ? node["scaleRatio"][0].as<float>() : 0.0f;
    info.heightRatio = node["scaleRatio"][1] ? node["scaleRatio"][1].as<float>() : 0.0f;

    std::string textureId = screenName + "." + node["id"].as<std::string>();
    mTextureInfo[textureId] = info;
}

void UILoadSystem::LoadTextInfo(const std::string& screenName, YAML::Node& node)
{
    TextInfo info;
    info.x = node["pos"][0] ? node["pos"][0].as<float>() : 0.0f;
    info.y = node["pos"][1] ? node["pos"][1].as<float>() : 0.0f;
    info.xRatio = node["posRatio"][0] ? node["posRatio"][0].as<float>() : 0.0f;
    info.yRatio = node["posRatio"][1] ? node["posRatio"][1].as<float>() : 0.0f;
    info.scale = node["scale"][0] ? node["scale"][0].as<float>() : 0.0f;
    info.scaleRatio = node["scaleRatio"][0] ? node["scaleRatio"][0].as<float>() : 0.0f;
    if (node["text"]) {
        for (auto text : node["text"]) {
            info.texts.emplace_back(text.as<std::string>());
        }
    }

    std::string textId = screenName + "." + node["id"].as<std::string>();
    mTextInfo[textId] = info;
}

bool UILoadSystem::SaveUIInfo(const std::string& path)
{
    YAML::Node root;

    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load UI yaml: " << path << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    for (auto screenIt = root.begin(); screenIt != root.end(); ++screenIt) {
        const std::string screenName = screenIt->first.as<std::string>();
        YAML::Node elements = screenIt->second;

        for (std::size_t i = 0; i < elements.size(); ++i) {
            YAML::Node node = elements[i];

            if (!node["id"] || !node["type"]) {
                continue;
            }

            const std::string id = node["id"].as<std::string>();
            const std::string type = node["type"].as<std::string>();
            const std::string mapId = screenName + "." + id;

            if (type == "texture") {
                auto it = mTextureInfo.find(mapId);
                if (it == mTextureInfo.end()) {
                    continue;
                }

                const TextureInfo& info = it->second;

                node["posRatio"][0] = info.xRatio;
                node["posRatio"][1] = info.yRatio;
                node["scaleRatio"][0] = info.widthRatio;
                node["scaleRatio"][1] = info.heightRatio;
            } else if (type == "text") {
                auto it = mTextInfo.find(mapId);
                if (it == mTextInfo.end()) {
                    continue;
                }

                const TextInfo& info = it->second;

                node["posRatio"][0] = info.xRatio;
                node["posRatio"][1] = info.yRatio;
                node["scaleRatio"][0] = info.scaleRatio;

                if (!node["scaleRatio"][1]) {
                    node["scaleRatio"][1] = 1.0f;
                }
            }
        }
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open UI yaml for writing: " << path << std::endl;
        return false;
    }

    file << root;
    return true;
}