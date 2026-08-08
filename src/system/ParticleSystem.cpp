#include "system/ParticleSystem.h"

#include "effect/particle/ParticleConfigLoader.h"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

float DegreesToRadians(float degrees)
{
    return glm::radians(degrees);
}

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }
    return glm::normalize(value);
}

glm::vec3 CreatePerpendicular(const glm::vec3& axis)
{
    const glm::vec3 reference = std::abs(axis.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                         : glm::vec3(1.0f, 0.0f, 0.0f);
    return SafeNormalize(glm::cross(axis, reference), glm::vec3(1.0f, 0.0f, 0.0f));
}

ParticleEmitterDefinition CreateDefaultEmitter()
{
    ParticleEmitterDefinition emitter;
    emitter.texturePath = "spark_dot.png";
    emitter.blendMode = ParticleBlendMode::Additive;
    emitter.renderMode = ParticleRenderMode::Billboard;
    emitter.directionMode = ParticleDirectionMode::Sphere;
    emitter.count = 8;
    emitter.lifetime = {0.25f, 0.50f};
    emitter.speed = {0.5f, 2.0f};
    emitter.startSize = {0.15f, 0.30f};
    emitter.endSizeMultiplier = 0.1f;
    emitter.rotationDegrees = {0.0f, 360.0f};
    emitter.angularVelocityDegrees = {-90.0f, 90.0f};
    emitter.startColor = glm::vec4(1.0f);
    emitter.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    return emitter;
}
} // namespace

ParticleSystem::ParticleSystem(std::size_t maxParticleCount)
    : mRandomEngine(std::random_device{}()),
      mMaxParticleCount(std::max<std::size_t>(1, maxParticleCount))
{
    mParticles.reserve(mMaxParticleCount);
}

bool ParticleSystem::LoadDefinitions(const std::string& filePath)
{
    std::unordered_map<std::string, ParticleEffectDefinition> loadedDefinitions;
    if (!ParticleConfigLoader::Load(filePath, loadedDefinitions)) {
        return false;
    }

    mDefinitions = std::move(loadedDefinitions);
    mDefinitionFilePath = filePath;
    return true;
}

bool ParticleSystem::ReloadDefinitions()
{
    if (mDefinitionFilePath.empty()) {
        return false;
    }

    return LoadDefinitions(mDefinitionFilePath);
}

bool ParticleSystem::SaveDefinitions() const
{
    if (mDefinitionFilePath.empty()) {
        return false;
    }

    return ParticleConfigLoader::Save(mDefinitionFilePath, mDefinitions);
}

bool ParticleSystem::Emit(const std::string& effectId, const ParticleSpawnContext& context)
{
    const auto definitionIterator = mDefinitions.find(effectId);
    if (definitionIterator == mDefinitions.end()) {
        std::cerr << "Particle effect was not found: " << effectId << '\n';
        return false;
    }

    for (const ParticleEmitterDefinition& emitterDefinition : definitionIterator->second.emitters) {
        EmitFromDefinition(emitterDefinition, context);
    }

    return true;
}

void ParticleSystem::EmitEmitter(
    const ParticleEmitterDefinition& definition,
    const ParticleSpawnContext& context)
{
    EmitFromDefinition(definition, context);
}

bool ParticleSystem::CreateEffect(const std::string& effectId)
{
    ParticleEffectDefinition definition;
    definition.displayName = effectId;
    definition.emitters.push_back(CreateDefaultEmitter());
    return CreateEffect(effectId, definition);
}

bool ParticleSystem::CreateEffect(
    const std::string& effectId,
    const ParticleEffectDefinition& definition)
{
    if (effectId.empty() || mDefinitions.find(effectId) != mDefinitions.end()) {
        return false;
    }

    ParticleEffectDefinition definitionToAdd = definition;
    if (definitionToAdd.displayName.empty()) {
        definitionToAdd.displayName = effectId;
    }
    mDefinitions.emplace(effectId, std::move(definitionToAdd));
    return true;
}

bool ParticleSystem::RemoveEffect(std::string_view effectId)
{
    return mDefinitions.erase(std::string(effectId)) > 0;
}

const ParticleEffectDefinition* ParticleSystem::FindEffect(std::string_view effectId) const
{
    const auto found = mDefinitions.find(std::string(effectId));
    return found == mDefinitions.end() ? nullptr : &found->second;
}

ParticleEffectDefinition* ParticleSystem::FindEffectMutable(std::string_view effectId)
{
    const auto found = mDefinitions.find(std::string(effectId));
    return found == mDefinitions.end() ? nullptr : &found->second;
}

std::vector<std::string> ParticleSystem::GetEffectIds() const
{
    std::vector<std::string> effectIds;
    effectIds.reserve(mDefinitions.size());

    for (const auto& [effectId, definition] : mDefinitions) {
        (void)definition;
        effectIds.push_back(effectId);
    }

    std::sort(effectIds.begin(), effectIds.end());
    return effectIds;
}

void ParticleSystem::Update(float deltaTime)
{
    if (deltaTime <= 0.0f) {
        return;
    }

    for (Particle& particle : mParticles) {
        particle.age += deltaTime;
        if (particle.age >= particle.lifetime) {
            continue;
        }

        particle.velocity += particle.gravityDirection * particle.gravity * deltaTime;

        if (particle.drag > 0.0f) {
            particle.velocity *= std::exp(-particle.drag * deltaTime);
        }

        particle.position += particle.velocity * deltaTime;
        particle.rotationRadians += particle.angularVelocityRadians * deltaTime;
    }

    mParticles.erase(
        std::remove_if(mParticles.begin(), mParticles.end(), [](const Particle& particle) {
            return particle.age >= particle.lifetime;
        }),
        mParticles.end());
}

void ParticleSystem::Clear()
{
    mParticles.clear();
}

void ParticleSystem::EmitFromDefinition(
    const ParticleEmitterDefinition& definition,
    const ParticleSpawnContext& context)
{
    if (mParticles.size() >= mMaxParticleCount) {
        return;
    }

    const glm::vec3 contextNormal = SafeNormalize(context.normal, glm::vec3(0.0f, 1.0f, 0.0f));
    const float spawnScale = std::max(0.0f, context.scale);
    const int availableParticleCount = static_cast<int>(mMaxParticleCount - mParticles.size());
    const int spawnCount = std::min(definition.count, availableParticleCount);

    for (int particleIndex = 0; particleIndex < spawnCount; ++particleIndex) {
        Particle particle;
        particle.position = context.position + definition.positionOffset * spawnScale;
        particle.velocity = CreateSpawnDirection(definition, context) * RandomFloat(definition.speed) * spawnScale;
        particle.gravityDirection = -contextNormal;

        particle.startColor = definition.startColor;
        particle.endColor = definition.endColor;
        particle.texturePath = definition.texturePath;
        particle.blendMode = definition.blendMode;
        particle.renderMode = definition.renderMode;

        particle.startSize = std::max(0.0f, RandomFloat(definition.startSize) * spawnScale);
        particle.endSize = std::max(0.0f, particle.startSize * definition.endSizeMultiplier);
        particle.lifetime = std::max(0.001f, RandomFloat(definition.lifetime));
        particle.rotationRadians = DegreesToRadians(RandomFloat(definition.rotationDegrees));
        particle.angularVelocityRadians = DegreesToRadians(RandomFloat(definition.angularVelocityDegrees));
        particle.gravity = definition.gravity;
        particle.drag = definition.drag;
        particle.velocityStretch = definition.velocityStretch;

        mParticles.push_back(std::move(particle));
    }
}

float ParticleSystem::RandomFloat(const ParticleFloatRange& range)
{
    if (range.max <= range.min) {
        return range.min;
    }

    std::uniform_real_distribution<float> distribution(range.min, range.max);
    return distribution(mRandomEngine);
}

glm::vec3 ParticleSystem::CreateSpawnDirection(
    const ParticleEmitterDefinition& definition,
    const ParticleSpawnContext& context)
{
    const glm::vec3 normal = SafeNormalize(context.normal, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 direction = SafeNormalize(context.direction, normal);

    switch (definition.directionMode) {
    case ParticleDirectionMode::Fixed:
        return direction;
    case ParticleDirectionMode::Hemisphere:
        return RandomHemisphereDirection(normal);
    case ParticleDirectionMode::Cone:
        return RandomConeDirection(direction, DegreesToRadians(definition.spreadAngleDegrees));
    case ParticleDirectionMode::Sphere:
    default:
        return RandomUnitVector();
    }
}

glm::vec3 ParticleSystem::RandomUnitVector()
{
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);

    const float z = unitDistribution(mRandomEngine) * 2.0f - 1.0f;
    const float angle = unitDistribution(mRandomEngine) * glm::two_pi<float>();
    const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));

    return {
        radius * std::cos(angle),
        radius * std::sin(angle),
        z,
    };
}

glm::vec3 ParticleSystem::RandomHemisphereDirection(const glm::vec3& normal)
{
    glm::vec3 direction = RandomUnitVector();
    if (glm::dot(direction, normal) < 0.0f) {
        direction = -direction;
    }
    return direction;
}

glm::vec3 ParticleSystem::RandomConeDirection(const glm::vec3& axis, float spreadAngleRadians)
{
    const glm::vec3 normalizedAxis = SafeNormalize(axis, glm::vec3(0.0f, 0.0f, 1.0f));
    const float clampedSpread = glm::clamp(spreadAngleRadians, 0.0f, glm::pi<float>());

    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    const float minimumCosine = std::cos(clampedSpread);
    const float cosineTheta = minimumCosine + (1.0f - minimumCosine) * unitDistribution(mRandomEngine);
    const float sineTheta = std::sqrt(std::max(0.0f, 1.0f - cosineTheta * cosineTheta));
    const float azimuth = unitDistribution(mRandomEngine) * glm::two_pi<float>();

    const glm::vec3 tangent = CreatePerpendicular(normalizedAxis);
    const glm::vec3 bitangent = glm::normalize(glm::cross(normalizedAxis, tangent));

    return SafeNormalize(
        normalizedAxis * cosineTheta + tangent * (std::cos(azimuth) * sineTheta) +
            bitangent * (std::sin(azimuth) * sineTheta),
        normalizedAxis);
}
