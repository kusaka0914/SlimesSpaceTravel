#include "UILoadSystem.h"
#include "system/text/JapaneseRubyGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>

UILoadSystem::UILoadSystem()
{
    Initialize();
}

void UILoadSystem::Initialize()
{
    ReloadUIInfo();
    LoadCustomUI();
}

bool UILoadSystem::ReloadUIInfo(const std::string& path)
{
    const auto previousTextureInfo = mTextureInfo;
    const auto previousTextInfo = mTextInfo;

    mTextureInfo.clear();
    mTextInfo.clear();

    try {
        LoadUIInfo(path);
    } catch (const YAML::Exception& exception) {
        mTextureInfo = previousTextureInfo;
        mTextInfo = previousTextInfo;
        std::cerr << "Failed to load UI yaml: " << path << std::endl;
        std::cerr << exception.what() << std::endl;
        return false;
    }

    return true;
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
    info.rotationDegrees =
        node["rotationDegrees"] ? node["rotationDegrees"].as<float>() : 0.0f;

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
    info.rubyScaleRatio =
        node["rubyScaleRatio"] ? node["rubyScaleRatio"].as<float>() : info.rubyScaleRatio;
    info.rubyGapRatio =
        node["rubyGapRatio"] ? node["rubyGapRatio"].as<float>() : info.rubyGapRatio;
    info.centerBased =
        node["centerBased"]
            ? node["centerBased"].as<bool>()
            : std::abs(info.xRatio - 0.5f) < 0.0001f;
    info.rotationDegrees =
        node["rotationDegrees"] ? node["rotationDegrees"].as<float>() : 0.0f;
    if (node["text"]) {
        for (auto text : node["text"]) {
            info.texts.emplace_back(text.as<std::string>());

            std::vector<RubyTextSegment> segments;
            std::string errorMessage;
            JapaneseRubyGenerator::Generate(info.texts.back(), segments, errorMessage);
            info.rubySegments.emplace_back(std::move(segments));
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
                node["rotationDegrees"] = info.rotationDegrees;
            } else if (type == "text") {
                auto it = mTextInfo.find(mapId);
                if (it == mTextInfo.end()) {
                    continue;
                }

                const TextInfo& info = it->second;

                node["posRatio"][0] = info.xRatio;
                node["posRatio"][1] = info.yRatio;
                node["scaleRatio"][0] = info.scaleRatio;
                node["rubyScaleRatio"] = info.rubyScaleRatio;
                node["rubyGapRatio"] = info.rubyGapRatio;
                node["centerBased"] = info.centerBased;
                node["rotationDegrees"] = info.rotationDegrees;

                if (!info.texts.empty()) {
                    YAML::Node texts(YAML::NodeType::Sequence);
                    for (const std::string& text : info.texts) {
                        texts.push_back(text);
                    }
                    node["text"] = texts;
                }

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

bool UILoadSystem::UpdateTextInfoContent(
    const std::string& mapId,
    std::size_t textIndex,
    const std::string& text)
{
    const auto textInfoIt = mTextInfo.find(mapId);
    if (textInfoIt == mTextInfo.end() ||
        textIndex >= textInfoIt->second.texts.size()) {
        return false;
    }

    TextInfo& textInfo = textInfoIt->second;
    textInfo.texts[textIndex] = text;
    textInfo.rubySegments.resize(textInfo.texts.size());

    std::string errorMessage;
    JapaneseRubyGenerator::Generate(
        text,
        textInfo.rubySegments[textIndex],
        errorMessage);
    return true;
}

std::string UILoadSystem::FindTextInfoKey(const TextInfo* textInfo) const
{
    if (!textInfo) {
        return std::string();
    }

    for (const auto& [key, candidate] : mTextInfo) {
        if (&candidate == textInfo) {
            return key;
        }
    }
    return std::string();
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
    element.displayName = element.id;
    mCustomScreenDisplayNames.try_emplace(element.screen, element.screen);

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

std::optional<std::size_t> UILoadSystem::DuplicateCustomElement(std::size_t index)
{
    if (index >= mCustomElements.size()) {
        return std::nullopt;
    }

    CustomElement duplicatedElement = mCustomElements[index];
    duplicatedElement.id =
        MakeUniqueCustomElementId(duplicatedElement.screen, duplicatedElement.id + "_copy");
    duplicatedElement.displayName =
        (duplicatedElement.displayName.empty()
             ? mCustomElements[index].id
             : duplicatedElement.displayName) +
        " コピー";

    constexpr float DuplicateOffsetRatio = 0.02f;
    duplicatedElement.xRatio =
        std::clamp(duplicatedElement.xRatio + DuplicateOffsetRatio, -0.5f, 1.5f);
    duplicatedElement.yRatio =
        std::clamp(duplicatedElement.yRatio + DuplicateOffsetRatio, -0.25f, 1.0f);

    mCustomElements.emplace_back(std::move(duplicatedElement));
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
        mCustomScreenDisplayNames.clear();
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load custom UI yaml: " << path << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    mCustomElements.clear();
    mCustomScreenDisplayNames.clear();
    mCustomElementVisibilityOverrides.clear();
    mCustomScreenVisibilityOverrides.clear();

    const YAML::Node screens = root["screens"];
    if (screens && screens.IsMap()) {
        for (const auto& screenEntry : screens) {
            const std::string screenId =
                screenEntry.first.as<std::string>();
            const YAML::Node screenNode = screenEntry.second;

            std::string displayName = screenId;
            if (screenNode.IsScalar()) {
                displayName = screenNode.as<std::string>();
            } else if (screenNode["name"]) {
                displayName = screenNode["name"].as<std::string>();
            }
            mCustomScreenDisplayNames[screenId] =
                displayName.empty() ? screenId : displayName;
        }
    }

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
        element.displayName =
            node["name"] ? node["name"].as<std::string>() : element.id;
        mCustomScreenDisplayNames.try_emplace(
            element.screen,
            element.screen);
        element.type =
            CustomElementTypeFromString(node["type"] ? node["type"].as<std::string>() : std::string("text"));
        element.visibleByDefault = node["visibleByDefault"] ? node["visibleByDefault"].as<bool>() : false;
        element.centerBased = node["centerBased"] ? node["centerBased"].as<bool>() : false;
        element.flipVertical = node["flipVertical"] ? node["flipVertical"].as<bool>() : true;
        element.zOrder = node["zOrder"] ? node["zOrder"].as<int>() : 0;
        element.rotationDegrees =
            node["rotationDegrees"] ? node["rotationDegrees"].as<float>() : 0.0f;

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
        element.shadowEnabled =
            node["shadowEnabled"] ? node["shadowEnabled"].as<bool>() : element.shadowEnabled;
        element.shadowOffsetXRatio =
            node["shadowOffsetXRatio"]
                ? node["shadowOffsetXRatio"].as<float>()
                : element.shadowOffsetXRatio;
        element.shadowOffsetYRatio =
            node["shadowOffsetYRatio"]
                ? node["shadowOffsetYRatio"].as<float>()
                : element.shadowOffsetYRatio;
        element.outlineEnabled =
            node["outlineEnabled"] ? node["outlineEnabled"].as<bool>() : element.outlineEnabled;
        element.outlineWidthRatio =
            node["outlineWidthRatio"]
                ? node["outlineWidthRatio"].as<float>()
                : element.outlineWidthRatio;

        if (node["color"] && node["color"].IsSequence() && node["color"].size() >= 4) {
            for (std::size_t i = 0; i < element.color.size(); ++i) {
                element.color[i] = node["color"][i].as<float>();
            }
        }
        if (node["shadowColor"] && node["shadowColor"].IsSequence() && node["shadowColor"].size() >= 4) {
            for (std::size_t i = 0; i < element.shadowColor.size(); ++i) {
                element.shadowColor[i] = node["shadowColor"][i].as<float>();
            }
        }
        if (node["outlineColor"] && node["outlineColor"].IsSequence() && node["outlineColor"].size() >= 4) {
            for (std::size_t i = 0; i < element.outlineColor.size(); ++i) {
                element.outlineColor[i] = node["outlineColor"][i].as<float>();
            }
        }

        mCustomElements.emplace_back(std::move(element));
    }

    return true;
}

bool UILoadSystem::SaveCustomUI(const std::string& path) const
{
    YAML::Node root;
    YAML::Node screens(YAML::NodeType::Map);
    YAML::Node elements(YAML::NodeType::Sequence);

    std::set<std::string> usedScreens;
    for (const CustomElement& element : mCustomElements) {
        usedScreens.insert(element.screen);
    }
    for (const std::string& screen : usedScreens) {
        screens[screen]["name"] =
            ResolveCustomScreenDisplayName(screen);
    }

    for (const CustomElement& element : mCustomElements) {
        YAML::Node node;
        node["screen"] = element.screen;
        node["id"] = element.id;
        node["name"] =
            element.displayName.empty()
                ? element.id
                : element.displayName;
        node["type"] = CustomElementTypeToString(element.type);
        node["visibleByDefault"] = element.visibleByDefault;
        node["centerBased"] = element.centerBased;
        node["flipVertical"] = element.flipVertical;
        node["zOrder"] = element.zOrder;
        node["rotationDegrees"] = element.rotationDegrees;
        node["posRatio"].push_back(element.xRatio);
        node["posRatio"].push_back(element.yRatio);
        node["sizeRatio"].push_back(element.widthRatio);
        node["sizeRatio"].push_back(element.heightRatio);
        node["textScaleRatio"] = element.textScaleRatio;
        node["text"] = element.text;
        node["texture"] = element.texturePath;
        node["shadowEnabled"] = element.shadowEnabled;
        node["shadowOffsetXRatio"] = element.shadowOffsetXRatio;
        node["shadowOffsetYRatio"] = element.shadowOffsetYRatio;
        node["outlineEnabled"] = element.outlineEnabled;
        node["outlineWidthRatio"] = element.outlineWidthRatio;

        for (float component : element.color) {
            node["color"].push_back(component);
        }
        for (float component : element.shadowColor) {
            node["shadowColor"].push_back(component);
        }
        for (float component : element.outlineColor) {
            node["outlineColor"].push_back(component);
        }

        elements.push_back(node);
    }

    root["coordinateBasis"] = "screenWidth";
    root["screens"] = screens;
    root["elements"] = elements;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open custom UI yaml for writing: " << path << std::endl;
        return false;
    }

    file << root;
    return true;
}

std::string UILoadSystem::ResolveCustomScreenDisplayName(
    const std::string& screen) const
{
    const auto found = mCustomScreenDisplayNames.find(screen);
    if (found == mCustomScreenDisplayNames.end() ||
        found->second.empty()) {
        return screen;
    }

    return found->second;
}

void UILoadSystem::SetCustomScreenDisplayName(
    const std::string& screen,
    const std::string& displayName)
{
    if (screen.empty()) {
        return;
    }

    mCustomScreenDisplayNames[screen] =
        displayName.empty() ? screen : displayName;
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
