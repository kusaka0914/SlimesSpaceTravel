#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

enum class ParticleBlendMode {
    Alpha,
    Additive,
};

enum class ParticleRenderMode {
    Billboard,
    VelocityAligned,
};

enum class ParticleDirectionMode {
    Fixed,
    Sphere,
    Hemisphere,
    Cone,
};

struct ParticleFloatRange {
    float min = 0.0f;
    float max = 0.0f;
};

struct ParticleEmitterDefinition {
    std::string texturePath;

    ParticleBlendMode blendMode = ParticleBlendMode::Additive;
    ParticleRenderMode renderMode = ParticleRenderMode::Billboard;
    ParticleDirectionMode directionMode = ParticleDirectionMode::Sphere;

    int count = 1;

    ParticleFloatRange lifetime{0.1f, 0.1f};
    ParticleFloatRange speed{0.0f, 0.0f};
    ParticleFloatRange startSize{0.1f, 0.1f};
    ParticleFloatRange rotationDegrees{0.0f, 0.0f};
    ParticleFloatRange angularVelocityDegrees{0.0f, 0.0f};

    float endSizeMultiplier = 0.0f;
    float spreadAngleDegrees = 180.0f;
    float gravity = 0.0f;
    float drag = 0.0f;
    float velocityStretch = 0.0f;

    glm::vec3 positionOffset{0.0f};
    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};
};

struct ParticleEffectDefinition {
    std::vector<ParticleEmitterDefinition> emitters;
};

struct ParticleSpawnContext {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec3 direction{0.0f, 0.0f, 1.0f};
    float scale = 1.0f;
};

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 gravityDirection{0.0f, -1.0f, 0.0f};

    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

    std::string texturePath;

    ParticleBlendMode blendMode = ParticleBlendMode::Additive;
    ParticleRenderMode renderMode = ParticleRenderMode::Billboard;

    float startSize = 0.1f;
    float endSize = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    float rotationRadians = 0.0f;
    float angularVelocityRadians = 0.0f;
    float gravity = 0.0f;
    float drag = 0.0f;
    float velocityStretch = 0.0f;
};
