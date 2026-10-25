#include "system/story/StorybookConfig.h"

#include "utils/AtomicFile.h"
#include "utils/YamlFile.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <yaml-cpp/yaml.h>

bool StorybookConfig::Load(const std::string& path)
{
    std::unordered_map<std::string, StorybookTrack> loadedTracks;
    try {
        const YAML::Node root = YamlFile::Load(path);
        const YAML::Node tracks = root["tracks"];
        if (!tracks || !tracks.IsMap()) {
            mTracks.clear();
            return true;
        }
        for (const auto& entry : tracks) {
            StorybookTrack track;
            const YAML::Node pages = entry.second["pages"];
            if (pages && pages.IsSequence()) {
                for (const YAML::Node& page : pages) {
                    track.pageImages.push_back(page ? page.as<std::string>() : "");
                }
            }
            loadedTracks[entry.first.as<std::string>()] = std::move(track);
        }
    } catch (const YAML::BadFile&) {
        mTracks.clear();
        return true;
    } catch (const std::exception&) {
        return false;
    }
    mTracks = std::move(loadedTracks);
    return true;
}

bool StorybookConfig::Save(const std::string& path) const
{
    try {
        const std::filesystem::path outputPath(path);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }
        std::string error;
        return AtomicFile::WriteText(outputPath, Serialize(), error);
    } catch (const std::exception&) {
        return false;
    }
}

std::string StorybookConfig::Serialize() const
{
    YAML::Emitter emitter;
    emitter << YAML::BeginMap << YAML::Key << "tracks" << YAML::Value << YAML::BeginMap;
    for (const auto& [id, track] : mTracks) {
        emitter << YAML::Key << id << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "pages" << YAML::Value << YAML::BeginSeq;
        for (const std::string& image : track.pageImages) {
            emitter << image;
        }
        emitter << YAML::EndSeq << YAML::EndMap;
    }
    emitter << YAML::EndMap << YAML::EndMap;
    if (!emitter.good()) {
        throw std::runtime_error(emitter.GetLastError());
    }
    return emitter.c_str();
}

std::string StorybookConfig::GetPageImage(const std::string& trackId, int pageIndex) const
{
    const auto track = mTracks.find(trackId);
    if (track == mTracks.end() || pageIndex < 0 || track->second.pageImages.empty()) {
        return "";
    }
    const int lastPageIndex = static_cast<int>(track->second.pageImages.size()) - 1;
    return track->second.pageImages[std::min(pageIndex, lastPageIndex)];
}

std::vector<std::string>& StorybookConfig::GetEditablePageImages(const std::string& trackId)
{
    return mTracks[trackId].pageImages;
}

void StorybookConfig::RemoveTrack(const std::string& trackId)
{
    mTracks.erase(trackId);
}
