#include "effect/particle/ParticleConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}

ParticleFloatRange ReadRange(const YAML::Node& node, const ParticleFloatRange& defaultValue)
{
    if (!node) {
        return defaultValue;
    }

    if (node.IsScalar()) {
        const float value = node.as<float>();
        return {value, value};
    }

    ParticleFloatRange result = defaultValue;
    if (node["min"]) {
        result.min = node["min"].as<float>();
    }
    if (node["max"]) {
        result.max = node["max"].as<float>();
    }

    if (result.min > result.max) {
        std::swap(result.min, result.max);
    }

    return result;
}

glm::vec3 ReadVec3(const YAML::Node& node, const glm::vec3& defaultValue)
{
    if (!node || !node.IsSequence() || node.size() < 3) {
        return defaultValue;
    }

    return {
        node[0].as<float>(),
        node[1].as<float>(),
        node[2].as<float>(),
    };
}

glm::vec4 ReadVec4(const YAML::Node& node, const glm::vec4& defaultValue)
{
    if (!node || !node.IsSequence() || node.size() < 4) {
        return defaultValue;
    }

    return {
        node[0].as<float>(),
        node[1].as<float>(),
        node[2].as<float>(),
        node[3].as<float>(),
    };
}

ParticleBlendMode ParseBlendMode(const std::string& value)
{
    return ToLower(value) == "alpha" ? ParticleBlendMode::Alpha : ParticleBlendMode::Additive;
}

ParticleRenderMode ParseRenderMode(const std::string& value)
{
    const std::string lowerValue = ToLower(value);
    return lowerValue == "velocityaligned" || lowerValue == "velocity_aligned"
               ? ParticleRenderMode::VelocityAligned
               : ParticleRenderMode::Billboard;
}

ParticleDirectionMode ParseDirectionMode(const std::string& value)
{
    const std::string lowerValue = ToLower(value);
    if (lowerValue == "fixed") {
        return ParticleDirectionMode::Fixed;
    }
    if (lowerValue == "hemisphere") {
        return ParticleDirectionMode::Hemisphere;
    }
    if (lowerValue == "cone") {
        return ParticleDirectionMode::Cone;
    }
    return ParticleDirectionMode::Sphere;
}

const char* ToString(ParticleBlendMode blendMode)
{
    return blendMode == ParticleBlendMode::Alpha ? "alpha" : "additive";
}

const char* ToString(ParticleRenderMode renderMode)
{
    return renderMode == ParticleRenderMode::VelocityAligned ? "velocityAligned" : "billboard";
}

const char* ToString(ParticleDirectionMode directionMode)
{
    switch (directionMode) {
    case ParticleDirectionMode::Fixed:
        return "fixed";
    case ParticleDirectionMode::Hemisphere:
        return "hemisphere";
    case ParticleDirectionMode::Cone:
        return "cone";
    case ParticleDirectionMode::Sphere:
    default:
        return "sphere";
    }
}

ParticleEmitterDefinition ReadEmitterDefinition(const YAML::Node& emitterNode)
{
    ParticleEmitterDefinition definition;

    definition.texturePath = ReadString(emitterNode, "texture", definition.texturePath);
    definition.blendMode = ParseBlendMode(ReadString(emitterNode, "blendMode", "additive"));
    definition.renderMode = ParseRenderMode(ReadString(emitterNode, "renderMode", "billboard"));
    definition.directionMode = ParseDirectionMode(ReadString(emitterNode, "directionMode", "sphere"));

    definition.count = std::max(0, ReadInt(emitterNode, "count", definition.count));
    definition.lifetime = ReadRange(emitterNode["lifetime"], definition.lifetime);
    definition.speed = ReadRange(emitterNode["speed"], definition.speed);
    definition.rotationDegrees = ReadRange(emitterNode["rotationDegrees"], definition.rotationDegrees);
    definition.angularVelocityDegrees =
        ReadRange(emitterNode["angularVelocityDegrees"], definition.angularVelocityDegrees);

    const YAML::Node sizeNode = emitterNode["size"];
    if (sizeNode) {
        definition.startSize = ReadRange(sizeNode["start"], definition.startSize);
        definition.endSizeMultiplier = ReadFloat(sizeNode, "endMultiplier", definition.endSizeMultiplier);
    }

    definition.spreadAngleDegrees = ReadFloat(emitterNode, "spreadAngleDegrees", definition.spreadAngleDegrees);
    definition.gravity = ReadFloat(emitterNode, "gravity", definition.gravity);
    definition.drag = std::max(0.0f, ReadFloat(emitterNode, "drag", definition.drag));
    definition.velocityStretch =
        std::max(0.0f, ReadFloat(emitterNode, "velocityStretch", definition.velocityStretch));

    definition.positionOffset = ReadVec3(emitterNode["positionOffset"], definition.positionOffset);
    definition.startColor = ReadVec4(emitterNode["colorStart"], definition.startColor);
    definition.endColor = ReadVec4(emitterNode["colorEnd"], definition.endColor);

    return definition;
}

void EmitRange(YAML::Emitter& emitter, const ParticleFloatRange& range)
{
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "min" << YAML::Value << range.min;
    emitter << YAML::Key << "max" << YAML::Value << range.max;
    emitter << YAML::EndMap;
}

void EmitVec3(YAML::Emitter& emitter, const glm::vec3& value)
{
    emitter << YAML::Flow << YAML::BeginSeq << value.x << value.y << value.z << YAML::EndSeq;
}

void EmitVec4(YAML::Emitter& emitter, const glm::vec4& value)
{
    emitter << YAML::Flow << YAML::BeginSeq << value.r << value.g << value.b << value.a << YAML::EndSeq;
}

void EmitEmitterDefinition(YAML::Emitter& emitter, const ParticleEmitterDefinition& definition)
{
    emitter << YAML::BeginMap;

    emitter << YAML::Key << "texture" << YAML::Value << definition.texturePath;
    emitter << YAML::Key << "blendMode" << YAML::Value << ToString(definition.blendMode);
    emitter << YAML::Key << "renderMode" << YAML::Value << ToString(definition.renderMode);
    emitter << YAML::Key << "directionMode" << YAML::Value << ToString(definition.directionMode);
    emitter << YAML::Key << "count" << YAML::Value << definition.count;

    emitter << YAML::Key << "lifetime" << YAML::Value;
    EmitRange(emitter, definition.lifetime);

    emitter << YAML::Key << "speed" << YAML::Value;
    EmitRange(emitter, definition.speed);

    emitter << YAML::Key << "size" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "start" << YAML::Value;
    EmitRange(emitter, definition.startSize);
    emitter << YAML::Key << "endMultiplier" << YAML::Value << definition.endSizeMultiplier;
    emitter << YAML::EndMap;

    emitter << YAML::Key << "rotationDegrees" << YAML::Value;
    EmitRange(emitter, definition.rotationDegrees);

    emitter << YAML::Key << "angularVelocityDegrees" << YAML::Value;
    EmitRange(emitter, definition.angularVelocityDegrees);

    emitter << YAML::Key << "spreadAngleDegrees" << YAML::Value << definition.spreadAngleDegrees;
    emitter << YAML::Key << "gravity" << YAML::Value << definition.gravity;
    emitter << YAML::Key << "drag" << YAML::Value << definition.drag;
    emitter << YAML::Key << "velocityStretch" << YAML::Value << definition.velocityStretch;

    emitter << YAML::Key << "positionOffset" << YAML::Value;
    EmitVec3(emitter, definition.positionOffset);

    emitter << YAML::Key << "colorStart" << YAML::Value;
    EmitVec4(emitter, definition.startColor);

    emitter << YAML::Key << "colorEnd" << YAML::Value;
    EmitVec4(emitter, definition.endColor);

    emitter << YAML::EndMap;
}
} // namespace

bool ParticleConfigLoader::Load(
    const std::string& filePath,
    std::unordered_map<std::string, ParticleEffectDefinition>& outDefinitions)
{
    try {
        const YAML::Node root = YAML::LoadFile(filePath);
        const YAML::Node effectsNode = root["particleEffects"];

        if (!effectsNode || !effectsNode.IsMap()) {
            std::cerr << "Particle config does not contain a particleEffects map: " << filePath << '\n';
            return false;
        }

        std::unordered_map<std::string, ParticleEffectDefinition> loadedDefinitions;

        for (const auto& effectEntry : effectsNode) {
            const std::string effectId = effectEntry.first.as<std::string>();
            const YAML::Node emittersNode = effectEntry.second["emitters"];

            if (!emittersNode || !emittersNode.IsSequence()) {
                std::cerr << "Particle effect '" << effectId << "' has no emitter sequence.\n";
                continue;
            }

            ParticleEffectDefinition effectDefinition;
            for (const YAML::Node& emitterNode : emittersNode) {
                ParticleEmitterDefinition emitterDefinition = ReadEmitterDefinition(emitterNode);
                if (emitterDefinition.texturePath.empty() || emitterDefinition.count <= 0) {
                    continue;
                }
                effectDefinition.emitters.push_back(std::move(emitterDefinition));
            }

            if (!effectDefinition.emitters.empty()) {
                loadedDefinitions[effectId] = std::move(effectDefinition);
            }
        }

        outDefinitions = std::move(loadedDefinitions);
        return true;
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load particle config '" << filePath << "': " << exception.what() << '\n';
        return false;
    }
}

bool ParticleConfigLoader::Save(
    const std::string& filePath,
    const std::unordered_map<std::string, ParticleEffectDefinition>& definitions)
{
    try {
        const std::filesystem::path outputPath(filePath);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        std::vector<std::string> effectIds;
        effectIds.reserve(definitions.size());

        for (const auto& [effectId, definition] : definitions) {
            (void)definition;
            effectIds.push_back(effectId);
        }

        std::sort(effectIds.begin(), effectIds.end());

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "particleEffects" << YAML::Value << YAML::BeginMap;

        for (const std::string& effectId : effectIds) {
            const ParticleEffectDefinition& effectDefinition = definitions.at(effectId);

            emitter << YAML::Key << effectId << YAML::Value << YAML::BeginMap;
            emitter << YAML::Key << "emitters" << YAML::Value << YAML::BeginSeq;

            for (const ParticleEmitterDefinition& emitterDefinition : effectDefinition.emitters) {
                EmitEmitterDefinition(emitter, emitterDefinition);
            }

            emitter << YAML::EndSeq;
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        if (!emitter.good()) {
            std::cerr << "Failed to serialize particle config: " << emitter.GetLastError() << '\n';
            return false;
        }

        std::ofstream output(filePath);
        if (!output) {
            std::cerr << "Failed to open particle config for writing: " << filePath << '\n';
            return false;
        }

        output << emitter.c_str() << '\n';
        return static_cast<bool>(output);
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save particle config '" << filePath << "': " << exception.what() << '\n';
        return false;
    }
}
