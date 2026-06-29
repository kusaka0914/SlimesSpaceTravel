#pragma once

#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

class UILoadSystem {
public:
    struct TextureInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float width = 0.0f;
        float widthRatio = 0.0f;
        float height = 0.0f;
        float heightRatio = 0.0f;
    };

    struct TextInfo {
        float x = 0.0f;
        float xRatio = 0.0f;
        float y = 0.0f;
        float yRatio = 0.0f;
        float scale = 0.0f;
        float scaleRatio = 0.0f;
        std::vector<std::string> texts;
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

    std::unordered_map<std::string, TextureInfo>& GetEditableTextureInfos() { return mTextureInfo; }

    std::unordered_map<std::string, TextInfo>& GetEditableTextInfos() { return mTextInfo; }

    bool SaveUIInfo(const std::string& path);

private:
    void LoadUIInfo(const std::string& path);
    void LoadTextureInfo(const std::string& screenName, YAML::Node& node);
    void LoadTextInfo(const std::string& screenName, YAML::Node& node);

private:
    std::unordered_map<std::string, TextureInfo> mTextureInfo;
    std::unordered_map<std::string, TextInfo> mTextInfo;
};