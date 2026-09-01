#include "system/story/StorybookConfig.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

bool StorybookConfig::Load(const std::string& path)
{
    mTracks.clear();
    try {
        const YAML::Node root = YAML::LoadFile(path);
        const YAML::Node tracks = root["tracks"];
        if (!tracks || !tracks.IsMap()) {
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
            mTracks[entry.first.as<std::string>()] = std::move(track);
        }
    } catch (const YAML::BadFile&) {
        return true;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool StorybookConfig::Save(const std::string& path) const
{
    try {
        const std::filesystem::path outputPath(path);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }
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
        std::ofstream output(path);
        output << emitter.c_str();
        return output.good();
    } catch (const std::exception&) {
        return false;
    }
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
