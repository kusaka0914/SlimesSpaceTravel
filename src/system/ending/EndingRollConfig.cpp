#include "system/ending/EndingRollConfig.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float fallback)
{
    return node[key] ? node[key].as<float>() : fallback;
}
}

bool EndingRollConfigIO::Load(EndingRollConfig& outConfig, const std::string& path)
{
    outConfig = {};
    try {
        const YAML::Node root = YAML::LoadFile(path);
        const YAML::Node credits = root["credits"];
        if (credits) {
            outConfig.creditsText = credits["text"] ? credits["text"].as<std::string>() : "";
            outConfig.creditsStartTime = std::max(0.0f, ReadFloat(credits, "start", 0.0f));
            outConfig.creditsStartYRatio = ReadFloat(credits, "startY", 1.1f);
            outConfig.creditsScrollSpeedRatio = std::max(0.001f, ReadFloat(credits, "scrollSpeed", 0.055f));
            outConfig.creditsTextScaleRatio = std::max(0.00002f, ReadFloat(credits, "textScale", 0.00055f));
        }
        outConfig.totalDuration = std::max(1.0f, ReadFloat(root, "duration", 30.0f));

        const YAML::Node events = root["imageEvents"];
        if (events && events.IsSequence()) {
            for (const YAML::Node& node : events) {
                EndingRollImageEvent event;
                event.imagePath = node["image"] ? node["image"].as<std::string>() : "";
                event.startTime = std::max(0.0f, ReadFloat(node, "start", 0.0f));
                event.visibleDuration = std::max(0.01f, ReadFloat(node, "duration", 1.5f));
                event.repeatInterval = std::max(event.visibleDuration, ReadFloat(node, "interval", 3.0f));
                event.repeatCount = std::max(1, node["repeatCount"] ? node["repeatCount"].as<int>() : 1);
                event.fadeInDuration = std::max(0.0f, ReadFloat(node, "fadeIn", 0.25f));
                event.fadeOutDuration = std::max(0.0f, ReadFloat(node, "fadeOut", 0.25f));
                event.xRatio = ReadFloat(node, "x", 0.5f);
                event.yRatio = ReadFloat(node, "y", 0.5f);
                event.widthRatio = std::max(0.01f, ReadFloat(node, "width", 0.2f));
                event.heightRatio = std::max(0.01f, ReadFloat(node, "height", 0.2f));
                outConfig.imageEvents.push_back(std::move(event));
            }
        }

        const YAML::Node endImage = root["endImage"];
        if (endImage) {
            outConfig.endImagePath = endImage["image"] ? endImage["image"].as<std::string>() : "";
            outConfig.endImageStartTime = std::max(0.0f, ReadFloat(endImage, "start", 24.0f));
            outConfig.endImageFadeInDuration = std::max(0.0f, ReadFloat(endImage, "fadeIn", 0.6f));
            outConfig.endImageHoldDuration = std::max(0.5f, ReadFloat(endImage, "holdDuration", 5.0f));
        }
    } catch (const YAML::BadFile&) {
        return true;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool EndingRollConfigIO::Save(const EndingRollConfig& config, const std::string& path)
{
    try {
        const std::filesystem::path outputPath(path);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "duration" << YAML::Value << config.totalDuration;
        emitter << YAML::Key << "credits" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "text" << YAML::Value << config.creditsText;
        emitter << YAML::Key << "start" << YAML::Value << config.creditsStartTime;
        emitter << YAML::Key << "startY" << YAML::Value << config.creditsStartYRatio;
        emitter << YAML::Key << "scrollSpeed" << YAML::Value << config.creditsScrollSpeedRatio;
        emitter << YAML::Key << "textScale" << YAML::Value << config.creditsTextScaleRatio;
        emitter << YAML::EndMap;
        emitter << YAML::Key << "imageEvents" << YAML::Value << YAML::BeginSeq;
        for (const EndingRollImageEvent& event : config.imageEvents) {
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "image" << YAML::Value << event.imagePath;
            emitter << YAML::Key << "start" << YAML::Value << event.startTime;
            emitter << YAML::Key << "duration" << YAML::Value << event.visibleDuration;
            emitter << YAML::Key << "interval" << YAML::Value << event.repeatInterval;
            emitter << YAML::Key << "repeatCount" << YAML::Value << event.repeatCount;
            emitter << YAML::Key << "fadeIn" << YAML::Value << event.fadeInDuration;
            emitter << YAML::Key << "fadeOut" << YAML::Value << event.fadeOutDuration;
            emitter << YAML::Key << "x" << YAML::Value << event.xRatio;
            emitter << YAML::Key << "y" << YAML::Value << event.yRatio;
            emitter << YAML::Key << "width" << YAML::Value << event.widthRatio;
            emitter << YAML::Key << "height" << YAML::Value << event.heightRatio;
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndSeq;
        emitter << YAML::Key << "endImage" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "image" << YAML::Value << config.endImagePath;
        emitter << YAML::Key << "start" << YAML::Value << config.endImageStartTime;
        emitter << YAML::Key << "fadeIn" << YAML::Value << config.endImageFadeInDuration;
        emitter << YAML::Key << "holdDuration" << YAML::Value << config.endImageHoldDuration;
        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        std::ofstream output(path);
        output << emitter.c_str();
        return output.good();
    } catch (const std::exception&) {
        return false;
    }
}

bool IsEndingRollImageVisible(const EndingRollImageEvent& event, float elapsedSeconds)
{
    if (elapsedSeconds < event.startTime) {
        return false;
    }
    const float sinceStart = elapsedSeconds - event.startTime;
    const int occurrence = static_cast<int>(sinceStart / event.repeatInterval);
    return occurrence < event.repeatCount &&
           sinceStart - static_cast<float>(occurrence) * event.repeatInterval < event.visibleDuration;
}

float CalculateEndingRollImageOpacity(const EndingRollImageEvent& event, float elapsedSeconds)
{
    if (!IsEndingRollImageVisible(event, elapsedSeconds)) {
        return 0.0f;
    }
    const float sinceStart = elapsedSeconds - event.startTime;
    const int occurrence = static_cast<int>(sinceStart / event.repeatInterval);
    const float localTime = sinceStart - static_cast<float>(occurrence) * event.repeatInterval;
    const float fadeIn = std::min(event.fadeInDuration, event.visibleDuration * 0.5f);
    const float fadeOut = std::min(event.fadeOutDuration, event.visibleDuration * 0.5f);
    const float fadeInOpacity = fadeIn > 0.0f ? localTime / fadeIn : 1.0f;
    const float fadeOutOpacity = fadeOut > 0.0f
        ? (event.visibleDuration - localTime) / fadeOut : 1.0f;
    return std::clamp(std::min(fadeInOpacity, fadeOutOpacity), 0.0f, 1.0f);
}
