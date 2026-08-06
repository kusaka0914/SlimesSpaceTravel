#include "system/sequence/SequenceLibrary.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace {
glm::vec3 ReadVec3(const YAML::Node& node, const glm::vec3& fallback)
{
    if (!node || !node.IsSequence() || node.size() != 3) {
        return fallback;
    }
    return {node[0].as<float>(), node[1].as<float>(), node[2].as<float>()};
}

void WriteVec3(YAML::Emitter& emitter, const glm::vec3& value)
{
    emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;
}

std::string Normalize(std::string value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        if (std::isspace(character) || character == '-' || character == '_') {
            continue;
        }
        result.push_back(static_cast<char>(std::tolower(character)));
    }
    return result;
}

SequenceClipType ParseClipType(const std::string& value)
{
    const std::string type = Normalize(value);
    if (type == "actorvisibility") {
        return SequenceClipType::ActorVisibility;
    }
    if (type == "playercontrol") {
        return SequenceClipType::PlayerControl;
    }
    if (type == "camera") {
        return SequenceClipType::Camera;
    }
    return SequenceClipType::ActorMove;
}

const char* ClipTypeToString(SequenceClipType type)
{
    switch (type) {
    case SequenceClipType::ActorVisibility:
        return "actorVisibility";
    case SequenceClipType::PlayerControl:
        return "playerControl";
    case SequenceClipType::Camera:
        return "camera";
    case SequenceClipType::ActorMove:
    default:
        return "actorMove";
    }
}

SequenceEasing ParseEasing(const std::string& value)
{
    const std::string easing = Normalize(value);
    if (easing == "linear") {
        return SequenceEasing::Linear;
    }
    if (easing == "easein") {
        return SequenceEasing::EaseIn;
    }
    if (easing == "easeout") {
        return SequenceEasing::EaseOut;
    }
    return SequenceEasing::EaseInOut;
}

const char* EasingToString(SequenceEasing easing)
{
    switch (easing) {
    case SequenceEasing::Linear:
        return "linear";
    case SequenceEasing::EaseIn:
        return "easeIn";
    case SequenceEasing::EaseOut:
        return "easeOut";
    case SequenceEasing::EaseInOut:
    default:
        return "easeInOut";
    }
}

void SortClips(GameplaySequence& sequence)
{
    std::stable_sort(
        sequence.clips.begin(),
        sequence.clips.end(),
        [](const SequenceClip& left, const SequenceClip& right) {
            return left.startTime < right.startTime;
        });
}
}

SequenceLibrary::SequenceLibrary(std::string filePath)
    : mFilePath(std::move(filePath))
{
}

bool SequenceLibrary::Load()
{
    mSequences.clear();

    try {
        const YAML::Node root = YAML::LoadFile(mFilePath);
        const YAML::Node sequencesNode = root["sequences"];
        if (!sequencesNode || !sequencesNode.IsMap()) {
            return true;
        }

        for (const auto& entry : sequencesNode) {
            GameplaySequence sequence;
            sequence.id = entry.first.as<std::string>();
            const YAML::Node sequenceNode = entry.second;
            sequence.displayName = sequenceNode["name"]
                                       ? sequenceNode["name"].as<std::string>()
                                       : sequence.id;
            sequence.loop = sequenceNode["loop"] ? sequenceNode["loop"].as<bool>() : false;

            const YAML::Node clipsNode = sequenceNode["clips"];
            if (clipsNode && clipsNode.IsSequence()) {
                for (const YAML::Node& clipNode : clipsNode) {
                    SequenceClip clip;
                    clip.type = ParseClipType(clipNode["type"] ? clipNode["type"].as<std::string>() : "actorMove");
                    clip.startTime =
                        clipNode["start"] ? std::max(0.0f, clipNode["start"].as<float>()) : 0.0f;
                    clip.duration =
                        clipNode["duration"] ? std::max(0.001f, clipNode["duration"].as<float>()) : 1.0f;

                    const YAML::Node targetNode = clipNode["target"];
                    if (targetNode) {
                        clip.actor.group =
                            targetNode["group"] ? targetNode["group"].as<std::string>() : "";
                        clip.actor.index = targetNode["index"] ? targetNode["index"].as<int>() : -1;
                    }

                    clip.fromPosition = ReadVec3(clipNode["from"], clip.fromPosition);
                    clip.toPosition = ReadVec3(clipNode["to"], clip.toPosition);
                    clip.easing =
                        ParseEasing(clipNode["easing"] ? clipNode["easing"].as<std::string>() : "easeInOut");
                    clip.visible = clipNode["visible"] ? clipNode["visible"].as<bool>() : true;
                    clip.playerControlEnabled =
                        clipNode["enabled"] ? clipNode["enabled"].as<bool>() : true;
                    clip.cameraSequenceId =
                        clipNode["sequence"] ? clipNode["sequence"].as<std::string>() : "";

                    sequence.clips.emplace_back(std::move(clip));
                }
            }

            SortClips(sequence);
            mSequences[sequence.id] = std::move(sequence);
        }
    } catch (const YAML::BadFile&) {
        return true;
    } catch (const std::exception& exception) {
        std::cerr << "Failed to load gameplay sequences: " << exception.what() << '\n';
        return false;
    }

    return true;
}

bool SequenceLibrary::Save() const
{
    try {
        const std::filesystem::path outputPath(mFilePath);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "sequences" << YAML::Value << YAML::BeginMap;

        for (const std::string& id : GetIds()) {
            const GameplaySequence& sequence = mSequences.at(id);
            emitter << YAML::Key << id << YAML::Value << YAML::BeginMap;
            emitter << YAML::Key << "name" << YAML::Value
                    << (sequence.displayName.empty() ? sequence.id : sequence.displayName);
            emitter << YAML::Key << "loop" << YAML::Value << sequence.loop;
            emitter << YAML::Key << "clips" << YAML::Value << YAML::BeginSeq;

            for (const SequenceClip& clip : sequence.clips) {
                emitter << YAML::BeginMap;
                emitter << YAML::Key << "type" << YAML::Value << ClipTypeToString(clip.type);
                emitter << YAML::Key << "start" << YAML::Value << clip.startTime;

                if (clip.type == SequenceClipType::ActorMove) {
                    emitter << YAML::Key << "duration" << YAML::Value << clip.duration;
                }

                if (clip.type == SequenceClipType::ActorMove ||
                    clip.type == SequenceClipType::ActorVisibility) {
                    emitter << YAML::Key << "target" << YAML::Value << YAML::BeginMap;
                    emitter << YAML::Key << "group" << YAML::Value << clip.actor.group;
                    emitter << YAML::Key << "index" << YAML::Value << clip.actor.index;
                    emitter << YAML::EndMap;
                }

                if (clip.type == SequenceClipType::ActorMove) {
                    emitter << YAML::Key << "from" << YAML::Value;
                    WriteVec3(emitter, clip.fromPosition);
                    emitter << YAML::Key << "to" << YAML::Value;
                    WriteVec3(emitter, clip.toPosition);
                    emitter << YAML::Key << "easing" << YAML::Value << EasingToString(clip.easing);
                } else if (clip.type == SequenceClipType::ActorVisibility) {
                    emitter << YAML::Key << "visible" << YAML::Value << clip.visible;
                } else if (clip.type == SequenceClipType::PlayerControl) {
                    emitter << YAML::Key << "enabled" << YAML::Value << clip.playerControlEnabled;
                } else if (clip.type == SequenceClipType::Camera) {
                    emitter << YAML::Key << "sequence" << YAML::Value << clip.cameraSequenceId;
                }

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
        std::cerr << "Failed to save gameplay sequences: " << exception.what() << '\n';
        return false;
    }
}

const GameplaySequence* SequenceLibrary::Find(std::string_view id) const
{
    const auto found = mSequences.find(std::string(id));
    return found == mSequences.end() ? nullptr : &found->second;
}

GameplaySequence* SequenceLibrary::FindMutable(std::string_view id)
{
    const auto found = mSequences.find(std::string(id));
    return found == mSequences.end() ? nullptr : &found->second;
}

bool SequenceLibrary::Create(std::string id)
{
    if (id.empty() || mSequences.contains(id)) {
        return false;
    }
    GameplaySequence sequence;
    sequence.id = std::move(id);
    sequence.displayName = sequence.id;
    mSequences[sequence.id] = std::move(sequence);
    return true;
}

GameplaySequence* SequenceLibrary::Duplicate(std::string_view id)
{
    const GameplaySequence* source = Find(id);
    if (!source) {
        return nullptr;
    }

    const std::string baseId = source->id + "_copy";
    std::string duplicatedId = baseId;
    int suffix = 2;
    while (mSequences.contains(duplicatedId)) {
        duplicatedId = baseId + "_" + std::to_string(suffix);
        ++suffix;
    }

    GameplaySequence duplicated = *source;
    duplicated.id = duplicatedId;
    duplicated.displayName =
        (source->displayName.empty() ? source->id : source->displayName) +
        " コピー";

    const auto [sequenceIt, wasInserted] =
        mSequences.emplace(duplicatedId, std::move(duplicated));
    return wasInserted ? &sequenceIt->second : nullptr;
}

bool SequenceLibrary::Rename(std::string_view currentId, std::string newId)
{
    if (newId.empty()) {
        return false;
    }

    const std::string currentIdString(currentId);
    auto found = mSequences.find(currentIdString);
    if (found == mSequences.end()) {
        return false;
    }
    if (currentIdString == newId) {
        return true;
    }
    if (mSequences.contains(newId)) {
        return false;
    }

    auto sequenceNode = mSequences.extract(found);
    sequenceNode.key() = newId;
    sequenceNode.mapped().id = std::move(newId);
    mSequences.insert(std::move(sequenceNode));
    return true;
}

bool SequenceLibrary::Remove(std::string_view id)
{
    return mSequences.erase(std::string(id)) > 0;
}

std::vector<std::string> SequenceLibrary::GetIds() const
{
    std::vector<std::string> ids;
    ids.reserve(mSequences.size());
    for (const auto& [id, sequence] : mSequences) {
        (void)sequence;
        ids.emplace_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}
