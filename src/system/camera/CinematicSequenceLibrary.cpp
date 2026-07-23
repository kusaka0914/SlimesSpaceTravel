#include "system/camera/CinematicSequenceLibrary.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <glm/common.hpp>
#include <utility>

namespace {
glm::vec3 ReadVec3(const YAML::Node& node, const glm::vec3& fallback)
{
    if (!node || !node.IsSequence() || node.size() != 3) {
        return fallback;
    }

    return glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
}

void WriteVec3(YAML::Emitter& emitter, const glm::vec3& value)
{
    emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;
}

std::string NormalizeToken(std::string value)
{
    std::string normalized;
    normalized.reserve(value.size());

    for (const unsigned char character : value) {
        if (std::isspace(character) || character == '-' || character == '_') {
            continue;
        }

        normalized.push_back(static_cast<char>(std::tolower(character)));
    }

    return normalized;
}

CameraEasing ParseEasing(const YAML::Node& node)
{
    if (!node) {
        return CameraEasing::EaseInOut;
    }

    const std::string value = NormalizeToken(node.as<std::string>());

    if (value == "linear") {
        return CameraEasing::Linear;
    }
    if (value == "easein") {
        return CameraEasing::EaseIn;
    }
    if (value == "easeout") {
        return CameraEasing::EaseOut;
    }

    return CameraEasing::EaseInOut;
}

const char* ToString(CameraEasing easing)
{
    switch (easing) {
    case CameraEasing::Linear:
        return "linear";
    case CameraEasing::EaseIn:
        return "easeIn";
    case CameraEasing::EaseOut:
        return "easeOut";
    case CameraEasing::EaseInOut:
        return "easeInOut";
    }

    return "easeInOut";
}

void SortKeyframes(CinematicSequence& sequence)
{
    std::stable_sort(sequence.keyframes.begin(), sequence.keyframes.end(),
                     [](const CinematicCameraKeyframe& left, const CinematicCameraKeyframe& right) {
                         return left.time < right.time;
                     });
}
} // namespace

CinematicSequenceLibrary::CinematicSequenceLibrary(std::string filePath)
    : mFilePath(std::move(filePath))
{
}

bool CinematicSequenceLibrary::Load()
{
    mSequences.clear();

    try {
        const YAML::Node root = YAML::LoadFile(mFilePath);
        const YAML::Node cinematicsNode = root["cinematics"];

        if (!cinematicsNode || !cinematicsNode.IsMap()) {
            return true;
        }

        for (const auto& entry : cinematicsNode) {
            CinematicSequence sequence;
            sequence.id = entry.first.as<std::string>();

            const YAML::Node sequenceNode = entry.second;
            sequence.loop = sequenceNode["loop"] ? sequenceNode["loop"].as<bool>() : false;
            sequence.endHoldDuration =
                sequenceNode["endHoldDuration"] ? std::max(0.0f, sequenceNode["endHoldDuration"].as<float>()) : 0.5f;

            const YAML::Node keyframesNode = sequenceNode["keyframes"];
            if (keyframesNode && keyframesNode.IsSequence()) {
                for (const YAML::Node& keyframeNode : keyframesNode) {
                    CinematicCameraKeyframe keyframe;
                    keyframe.time = keyframeNode["time"] ? std::max(0.0f, keyframeNode["time"].as<float>()) : 0.0f;
                    keyframe.pose.position = ReadVec3(keyframeNode["position"], keyframe.pose.position);
                    keyframe.pose.target = ReadVec3(keyframeNode["target"], keyframe.pose.target);
                    keyframe.pose.up = ReadVec3(keyframeNode["up"], keyframe.pose.up);
                    keyframe.pose.fieldOfViewDegrees =
                        keyframeNode["fieldOfView"]
                            ? glm::clamp(keyframeNode["fieldOfView"].as<float>(), 10.0f, 120.0f)
                            : 60.0f;
                    keyframe.easing = ParseEasing(keyframeNode["easing"]);
                    sequence.keyframes.push_back(keyframe);
                }
            }

            SortKeyframes(sequence);
            mSequences[sequence.id] = std::move(sequence);
        }
    } catch (const std::exception& exception) {
        std::cerr << "Failed to load cinematic camera data: " << exception.what() << '\n';
        return false;
    }

    return true;
}

bool CinematicSequenceLibrary::Save() const
{
    try {
        const std::filesystem::path outputPath(mFilePath);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "cinematics" << YAML::Value << YAML::BeginMap;

        const std::vector<std::string> sequenceIds = GetSequenceIds();
        for (const std::string& sequenceId : sequenceIds) {
            const CinematicSequence& sequence = mSequences.at(sequenceId);

            emitter << YAML::Key << sequence.id << YAML::Value << YAML::BeginMap;
            emitter << YAML::Key << "loop" << YAML::Value << sequence.loop;
            emitter << YAML::Key << "endHoldDuration" << YAML::Value << sequence.endHoldDuration;
            emitter << YAML::Key << "keyframes" << YAML::Value << YAML::BeginSeq;

            for (const CinematicCameraKeyframe& keyframe : sequence.keyframes) {
                emitter << YAML::BeginMap;
                emitter << YAML::Key << "time" << YAML::Value << keyframe.time;

                emitter << YAML::Key << "position" << YAML::Value;
                WriteVec3(emitter, keyframe.pose.position);

                emitter << YAML::Key << "target" << YAML::Value;
                WriteVec3(emitter, keyframe.pose.target);

                emitter << YAML::Key << "up" << YAML::Value;
                WriteVec3(emitter, keyframe.pose.up);

                emitter << YAML::Key << "fieldOfView" << YAML::Value << keyframe.pose.fieldOfViewDegrees;
                emitter << YAML::Key << "easing" << YAML::Value << ToString(keyframe.easing);
                emitter << YAML::EndMap;
            }

            emitter << YAML::EndSeq;
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        std::ofstream output(mFilePath);
        if (!output) {
            return false;
        }

        output << emitter.c_str() << '\n';
        return static_cast<bool>(output);
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save cinematic camera data: " << exception.what() << '\n';
        return false;
    }
}

const CinematicSequence* CinematicSequenceLibrary::Find(std::string_view sequenceId) const
{
    const auto found = mSequences.find(std::string(sequenceId));
    return found == mSequences.end() ? nullptr : &found->second;
}

CinematicSequence* CinematicSequenceLibrary::FindMutable(std::string_view sequenceId)
{
    const auto found = mSequences.find(std::string(sequenceId));
    return found == mSequences.end() ? nullptr : &found->second;
}

bool CinematicSequenceLibrary::Create(std::string sequenceId)
{
    if (sequenceId.empty() || mSequences.find(sequenceId) != mSequences.end()) {
        return false;
    }

    CinematicSequence sequence;
    sequence.id = std::move(sequenceId);
    mSequences[sequence.id] = std::move(sequence);
    return true;
}

bool CinematicSequenceLibrary::Remove(std::string_view sequenceId)
{
    return mSequences.erase(std::string(sequenceId)) > 0;
}

std::vector<std::string> CinematicSequenceLibrary::GetSequenceIds() const
{
    std::vector<std::string> ids;
    ids.reserve(mSequences.size());

    for (const auto& [sequenceId, sequence] : mSequences) {
        (void)sequence;
        ids.push_back(sequenceId);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}
