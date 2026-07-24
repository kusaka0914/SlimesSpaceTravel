#include "UILoadSystem.h"
#include <algorithm>
#include <fstream>
#include <iostream>

UILoadSystem::UILoadSystem()
{
    Initialize();
}

void UILoadSystem::Initialize()
{
    LoadUIInfo("../assets/data/ui/ui.yaml");
    LoadCustomUI();
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

std::size_t UILoadSystem::AddCustomElement(
    CustomElementType type,
    const std::string& screen,
    const std::string& requestedId)
{
    CustomElement element;
    element.type = type;
    element.screen = screen.empty() ? "custom" : screen;
    element.id = MakeUniqueCustomElementId(element.screen, requestedId.empty() ? "element" : requestedId);

    if (type == CustomElementType::Image || type == CustomElementType::Panel) {
        element.centerBased = false;
        element.xRatio = 0.4f;
        element.yRatio = 0.225f;
    } else {
        element.centerBased = true;
    }

    mCustomElements.emplace_back(std::move(element));
    return mCustomElements.size() - 1;
}

bool UILoadSystem::RemoveCustomElement(std::size_t index)
{
    if (index >= mCustomElements.size()) {
        return false;
    }

    const CustomElement& element = mCustomElements[index];
    mCustomElementVisibilityOverrides.erase(MakeCustomElementKey(element.screen, element.id));
    mCustomElements.erase(mCustomElements.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool UILoadSystem::LoadCustomUI(const std::string& path)
{
    YAML::Node root;

    try {
        root = YAML::LoadFile(path);
    } catch (const YAML::BadFile&) {
        mCustomElements.clear();
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load custom UI yaml: " << path << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    mCustomElements.clear();
    mCustomElementVisibilityOverrides.clear();
    mCustomScreenVisibilityOverrides.clear();

    const YAML::Node elements = root["elements"];
    if (!elements || !elements.IsSequence()) {
        return true;
    }

    const bool usesScreenWidthBasis =
        root["coordinateBasis"] && root["coordinateBasis"].as<std::string>() == "screenWidth";
    constexpr float LegacyHeightToWidthRatio = 450.0f / 800.0f;

    for (const YAML::Node& node : elements) {
        if (!node["id"]) {
            continue;
        }

        CustomElement element;
        element.screen = node["screen"] ? node["screen"].as<std::string>() : "custom";
        element.id = node["id"].as<std::string>();
        element.type =
            CustomElementTypeFromString(node["type"] ? node["type"].as<std::string>() : std::string("text"));
        element.visibleByDefault = node["visibleByDefault"] ? node["visibleByDefault"].as<bool>() : false;
        element.centerBased = node["centerBased"] ? node["centerBased"].as<bool>() : false;
        element.flipVertical = node["flipVertical"] ? node["flipVertical"].as<bool>() : true;
        element.zOrder = node["zOrder"] ? node["zOrder"].as<int>() : 0;

        if (node["posRatio"] && node["posRatio"].IsSequence() && node["posRatio"].size() >= 2) {
            element.xRatio = node["posRatio"][0].as<float>();
            element.yRatio = node["posRatio"][1].as<float>();
        }

        if (node["sizeRatio"] && node["sizeRatio"].IsSequence() && node["sizeRatio"].size() >= 2) {
            element.widthRatio = node["sizeRatio"][0].as<float>();
            element.heightRatio = node["sizeRatio"][1].as<float>();
        }

        if (!usesScreenWidthBasis) {
            element.yRatio *= LegacyHeightToWidthRatio;
            element.heightRatio *= LegacyHeightToWidthRatio;
        }

        element.textScaleRatio =
            node["textScaleRatio"] ? node["textScaleRatio"].as<float>() : element.textScaleRatio;
        element.text = node["text"] ? node["text"].as<std::string>() : element.text;
        element.texturePath = node["texture"] ? node["texture"].as<std::string>() : "";

        if (node["color"] && node["color"].IsSequence() && node["color"].size() >= 4) {
            for (std::size_t i = 0; i < element.color.size(); ++i) {
                element.color[i] = node["color"][i].as<float>();
            }
        }

        mCustomElements.emplace_back(std::move(element));
    }

    return true;
}

bool UILoadSystem::SaveCustomUI(const std::string& path) const
{
    YAML::Node root;
    YAML::Node elements(YAML::NodeType::Sequence);

    for (const CustomElement& element : mCustomElements) {
        YAML::Node node;
        node["screen"] = element.screen;
        node["id"] = element.id;
        node["type"] = CustomElementTypeToString(element.type);
        node["visibleByDefault"] = element.visibleByDefault;
        node["centerBased"] = element.centerBased;
        node["flipVertical"] = element.flipVertical;
        node["zOrder"] = element.zOrder;
        node["posRatio"].push_back(element.xRatio);
        node["posRatio"].push_back(element.yRatio);
        node["sizeRatio"].push_back(element.widthRatio);
        node["sizeRatio"].push_back(element.heightRatio);
        node["textScaleRatio"] = element.textScaleRatio;
        node["text"] = element.text;
        node["texture"] = element.texturePath;

        for (float component : element.color) {
            node["color"].push_back(component);
        }

        elements.push_back(node);
    }

    root["coordinateBasis"] = "screenWidth";
    root["elements"] = elements;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open custom UI yaml for writing: " << path << std::endl;
        return false;
    }

    file << root;
    return true;
}

void UILoadSystem::SetCustomElementVisible(
    const std::string& screen,
    const std::string& id,
    bool visible)
{
    mCustomElementVisibilityOverrides[MakeCustomElementKey(screen, id)] = visible;
}

void UILoadSystem::SetCustomScreenVisible(const std::string& screen, bool visible)
{
    mCustomScreenVisibilityOverrides[screen] = visible;
}

void UILoadSystem::ClearCustomVisibilityOverrides()
{
    mCustomElementVisibilityOverrides.clear();
    mCustomScreenVisibilityOverrides.clear();
}

bool UILoadSystem::IsCustomElementVisible(const CustomElement& element) const
{
    const auto elementIt =
        mCustomElementVisibilityOverrides.find(MakeCustomElementKey(element.screen, element.id));
    if (elementIt != mCustomElementVisibilityOverrides.end()) {
        return elementIt->second;
    }

    const auto screenIt = mCustomScreenVisibilityOverrides.find(element.screen);
    if (screenIt != mCustomScreenVisibilityOverrides.end()) {
        return screenIt->second;
    }

    return element.visibleByDefault;
}

const char* UILoadSystem::CustomElementTypeToString(CustomElementType type)
{
    switch (type) {
    case CustomElementType::Image:
        return "image";
    case CustomElementType::Panel:
        return "panel";
    case CustomElementType::Text:
    default:
        return "text";
    }
}

UILoadSystem::CustomElementType UILoadSystem::CustomElementTypeFromString(const std::string& type)
{
    if (type == "image") {
        return CustomElementType::Image;
    }
    if (type == "panel") {
        return CustomElementType::Panel;
    }
    return CustomElementType::Text;
}

std::string UILoadSystem::MakeUniqueCustomElementId(
    const std::string& screen,
    const std::string& requestedId) const
{
    const std::string baseId = requestedId.empty() ? "element" : requestedId;
    std::string candidate = baseId;
    int suffix = 2;

    const auto exists = [this, &screen](const std::string& id) {
        return std::any_of(mCustomElements.begin(), mCustomElements.end(), [&](const CustomElement& element) {
            return element.screen == screen && element.id == id;
        });
    };

    while (exists(candidate)) {
        candidate = baseId + std::to_string(suffix++);
    }

    return candidate;
}

std::string UILoadSystem::MakeCustomElementKey(const std::string& screen, const std::string& id)
{
    return screen + "." + id;
}
