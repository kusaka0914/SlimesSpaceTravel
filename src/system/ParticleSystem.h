#pragma once

#include "effect/particle/ParticleTypes.h"

#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class ParticleSystem {
public:
    explicit ParticleSystem(std::size_t maxParticleCount = 4096);

    bool LoadDefinitions(const std::string& filePath);
    bool ReloadDefinitions();
    bool SaveDefinitions() const;

    bool Emit(const std::string& effectId, const ParticleSpawnContext& context);
    void EmitEmitter(const ParticleEmitterDefinition& definition, const ParticleSpawnContext& context);

    bool CreateEffect(const std::string& effectId);
    bool CreateEffect(const std::string& effectId, const ParticleEffectDefinition& definition);
    bool RemoveEffect(std::string_view effectId);

    const ParticleEffectDefinition* FindEffect(std::string_view effectId) const;
    ParticleEffectDefinition* FindEffectMutable(std::string_view effectId);
    std::vector<std::string> GetEffectIds() const;

    void Update(float deltaTime);
    void Clear();

    const std::string& GetDefinitionFilePath() const { return mDefinitionFilePath; }
    const std::vector<Particle>& GetParticles() const { return mParticles; }
    std::size_t GetParticleCount() const { return mParticles.size(); }
    std::size_t GetMaxParticleCount() const { return mMaxParticleCount; }

private:
    void EmitFromDefinition(const ParticleEmitterDefinition& definition, const ParticleSpawnContext& context);

    float RandomFloat(const ParticleFloatRange& range);
    glm::vec3 CreateSpawnDirection(
        const ParticleEmitterDefinition& definition,
        const ParticleSpawnContext& context);
    glm::vec3 RandomUnitVector();
    glm::vec3 RandomHemisphereDirection(const glm::vec3& normal);
    glm::vec3 RandomConeDirection(const glm::vec3& axis, float spreadAngleRadians);

private:
    std::unordered_map<std::string, ParticleEffectDefinition> mDefinitions;
    std::vector<Particle> mParticles;
    std::mt19937 mRandomEngine;
    std::size_t mMaxParticleCount;
    std::string mDefinitionFilePath;
};
