#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct StorybookTrack {
    std::vector<std::string> pageImages;
};

class StorybookConfig {
public:
    static constexpr const char* DefaultPath = "../assets/data/sequences/storybook.yaml";

    bool Load(const std::string& path = DefaultPath);
    bool Save(const std::string& path = DefaultPath) const;
    std::string GetPageImage(const std::string& trackId, int pageIndex) const;
    std::vector<std::string>& GetEditablePageImages(const std::string& trackId);
    void RemoveTrack(const std::string& trackId);

private:
    std::unordered_map<std::string, StorybookTrack> mTracks;
};
