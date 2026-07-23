#include "effect/particle/ParticleConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <utility>

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
    return ToLower(value) == "velocityaligned" || ToLower(value) == "velocity_aligned"
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
    definition.velocityStretch = std::max(0.0f, ReadFloat(emitterNode, "velocityStretch", definition.velocityStretch));

    definition.positionOffset = ReadVec3(emitterNode["positionOffset"], definition.positionOffset);
    definition.startColor = ReadVec4(emitterNode["colorStart"], definition.startColor);
    definition.endColor = ReadVec4(emitterNode["colorEnd"], definition.endColor);

    return definition;
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
        return !outDefinitions.empty();
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load particle config '" << filePath << "': " << exception.what() << '\n';
        return false;
    }
}
