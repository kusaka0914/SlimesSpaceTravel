#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "text/RubyText.h"
#include <yaml-cpp/yaml.h>

class UILoadSystem {
public:
    enum class CustomElementType {
        Text,
        Image,
        Panel,
    };

    struct TextureInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float width = 0.0f;
        float widthRatio = 0.0f;
        float height = 0.0f;
        float heightRatio = 0.0f;
        float rotationDegrees = 0.0f;
    };

    struct TextInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float scale = 0.0f;
        float scaleRatio = 0.0f;
        float rubyScaleRatio = 0.48f;
        float rubyGapRatio = 0.0f;
        bool centerBased = false;
        float rotationDegrees = 0.0f;
        std::vector<std::string> texts;
        std::vector<std::vector<RubyTextSegment>> rubySegments;
    };

    struct CustomElement {
        std::string screen = "custom";
        std::string id = "element";
        std::string displayName = "element";
        CustomElementType type = CustomElementType::Text;
        bool visibleByDefault = false;
        bool centerBased = false;
        bool flipVertical = true;
        int zOrder = 0;
        float xRatio = 0.5f;
        float yRatio = 0.28125f;
        float widthRatio = 0.2f;
        float heightRatio = 0.05625f;
        float rotationDegrees = 0.0f;
        float textScaleRatio = 0.0007f;
        std::string text = "New Text";
        std::string texturePath;
        bool usesInputDeviceVariants = false;
        std::string keyboardText;
        std::string gameControllerText;
        std::string keyboardModifierText;
        std::string gameControllerModifierText;
        std::string keyboardTexturePath;
        std::string gameControllerTexturePath;
        bool keyboardFlipVertical = true;
        bool gameControllerFlipVertical = true;
        std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
        bool shadowEnabled = false;
        float shadowOffsetXRatio = 0.0015f;
        float shadowOffsetYRatio = 0.0015f;
        std::array<float, 4> shadowColor = {0.0f, 0.0f, 0.0f, 0.65f};
        bool outlineEnabled = false;
        float outlineWidthRatio = 0.001f;
        std::array<float, 4> outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    UILoadSystem();

    void Initialize();

    const TextureInfo* GetTextureInfo(const std::string& screenName, const std::string& id) const
    {
        std::string mapId = screenName + "." + id;
        auto it = mTextureInfo.find(mapId);
        return (it != mTextureInfo.end()) ? &it->second : nullptr;
    }

    const TextInfo* GetTextInfo(const std::string& screenName, const std::string& id) const
    {
        std::string mapId = screenName + "." + id;
        auto it = mTextInfo.find(mapId);
        return (it != mTextInfo.end()) ? &it->second : nullptr;
    }

    std::string FindTextInfoKey(const TextInfo* textInfo) const;

    std::unordered_map<std::string, TextureInfo>& GetEditableTextureInfos() { return mTextureInfo; }
    const std::unordered_map<std::string, TextureInfo>& GetEditableTextureInfos() const { return mTextureInfo; }

    std::unordered_map<std::string, TextInfo>& GetEditableTextInfos() { return mTextInfo; }
    const std::unordered_map<std::string, TextInfo>& GetEditableTextInfos() const { return mTextInfo; }

    bool ReloadUIInfo(const std::string& path = "../assets/data/ui/ui.yaml");
    bool SaveUIInfo(const std::string& path);
    bool UpdateTextInfoContent(
        const std::string& mapId,
        std::size_t textIndex,
        const std::string& text);

    std::vector<CustomElement>& GetCustomElements() { return mCustomElements; }
    const std::vector<CustomElement>& GetCustomElements() const { return mCustomElements; }

    std::string ResolveCustomScreenDisplayName(
        const std::string& screen) const;
    void SetCustomScreenDisplayName(
        const std::string& screen,
        const std::string& displayName);

    std::size_t AddCustomElement(CustomElementType type, const std::string& screen, const std::string& requestedId);
    std::optional<std::size_t> DuplicateCustomElement(std::size_t index);
    bool RemoveCustomElement(std::size_t index);

    bool LoadCustomUI(const std::string& path = "../assets/data/ui/custom_ui.yaml");
    bool SaveCustomUI(const std::string& path = "../assets/data/ui/custom_ui.yaml") const;

    void SetCustomElementVisible(const std::string& screen, const std::string& id, bool visible);
    void SetCustomScreenVisible(const std::string& screen, bool visible);
    void ClearCustomVisibilityOverrides();
    bool IsCustomElementVisible(const CustomElement& element) const;

    static const char* CustomElementTypeToString(CustomElementType type);
    static CustomElementType CustomElementTypeFromString(const std::string& type);

private:
    void LoadUIInfo(const std::string& path);
    void LoadTextureInfo(const std::string& screenName, YAML::Node& node);
    void LoadTextInfo(const std::string& screenName, YAML::Node& node);
    std::string MakeUniqueCustomElementId(const std::string& screen, const std::string& requestedId) const;
    static std::string MakeCustomElementKey(const std::string& screen, const std::string& id);

private:
    std::unordered_map<std::string, TextureInfo> mTextureInfo;
    std::unordered_map<std::string, TextInfo> mTextInfo;
    std::vector<CustomElement> mCustomElements;
    std::unordered_map<std::string, std::string> mCustomScreenDisplayNames;
    std::unordered_map<std::string, bool> mCustomElementVisibilityOverrides;
    std::unordered_map<std::string, bool> mCustomScreenVisibilityOverrides;
};
