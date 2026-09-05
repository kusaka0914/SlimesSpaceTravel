#include <GL/glew.h>

#include "gfx/EnvironmentDecorationRenderer.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Crystal.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "gfx/DirectionalShadowShader.h"
#include "gfx/Shader3D.h"
#include "system/MeshLoadSystem.h"
#include "system/mesh/LoadedMesh.h"
#include "system/mesh/LoadedModel.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>

namespace {
struct DecorationDefinition {
    const char* modelPath;
    int baseInstanceCount;
    int clusterCount;
    float clusterSpread;
    float minimumScale;
    float maximumScale;
    float obstacleClearance;
    float maximumDrawDistance;
};

constexpr std::array GrasslandDecorationDefinitions = {
    DecorationDefinition{"NatureAssets/grass.fbx", 180, 14, 0.24f, 0.16f, 0.29f, 0.38f, 32.0f},
    DecorationDefinition{"NatureAssets/flower_yellowA.fbx", 32, 6, 0.14f, 0.19f, 0.29f, 0.50f, 27.0f},
    DecorationDefinition{"NatureAssets/flower_redA.fbx", 24, 5, 0.13f, 0.18f, 0.28f, 0.50f, 27.0f},
    DecorationDefinition{"NatureAssets/plant_bushSmall.fbx", 18, 7, 0.20f, 0.18f, 0.32f, 0.75f, 38.0f},
    DecorationDefinition{"NatureAssets/rock_smallA.fbx", 18, 10, 0.32f, 0.18f, 0.33f, 0.90f, 44.0f},
    DecorationDefinition{"NatureAssets/rock_largeA.fbx", 5, 5, 0.25f, 0.20f, 0.32f, 1.65f, 58.0f},
    DecorationDefinition{"NatureAssets/tree_small.fbx", 8, 6, 0.20f, 0.36f, 0.55f, 2.25f, 66.0f},
};

constexpr std::array RockyDecorationDefinitions = {
    DecorationDefinition{"NatureAssets/rock_smallA.fbx", 34, 12, 0.31f, 0.18f, 0.36f, 0.85f, 44.0f},
    DecorationDefinition{"NatureAssets/rock_largeA.fbx", 9, 7, 0.27f, 0.22f, 0.38f, 1.65f, 60.0f},
    DecorationDefinition{"NatureAssets/stone_tallA.fbx", 7, 5, 0.22f, 0.18f, 0.31f, 1.25f, 62.0f},
    DecorationDefinition{"NatureAssets/plant_flatShort.fbx", 12, 5, 0.19f, 0.16f, 0.27f, 0.60f, 31.0f},
};

void AppendActorPositions(const auto& actors, std::vector<glm::vec3>& positions)
{
    for (const Actor* actor : actors) {
        if (actor && actor->GetIsActive()) {
            positions.push_back(actor->GetRenderPosition());
        }
    }
}

std::vector<glm::vec3> CollectOccupiedPositions(const Planet& planet)
{
    std::vector<glm::vec3> positions;
    AppendActorPositions(planet.GetBoats(), positions);
    AppendActorPositions(planet.GetCrystals(), positions);
    AppendActorPositions(planet.GetNPCs(), positions);
    AppendActorPositions(planet.GetPlatforms(), positions);
    AppendActorPositions(planet.GetStageObjects(), positions);
    if (planet.GetKey() && planet.GetKey()->GetIsActive()) {
        positions.push_back(planet.GetKey()->GetRenderPosition());
    }
    if (planet.GetStar() && planet.GetStar()->GetIsActive()) {
        positions.push_back(planet.GetStar()->GetRenderPosition());
    }
    return positions;
}

bool IsClearOfOccupiedPositions(
    const glm::vec3& position,
    const std::vector<glm::vec3>& occupiedPositions,
    float clearance)
{
    const float clearanceSquared = clearance * clearance;
    return std::none_of(
        occupiedPositions.begin(),
        occupiedPositions.end(),
        [&position, clearanceSquared](const glm::vec3& occupiedPosition) {
            const glm::vec3 separation = position - occupiedPosition;
            return glm::dot(separation, separation) < clearanceSquared;
        });
}

glm::vec3 CreateRandomUnitVector(
    std::mt19937& randomGenerator,
    std::uniform_real_distribution<float>& signedDistribution)
{
    glm::vec3 direction;
    do {
        direction = glm::vec3(
            signedDistribution(randomGenerator),
            signedDistribution(randomGenerator),
            signedDistribution(randomGenerator));
    } while (glm::dot(direction, direction) <= 0.0001f);
    return glm::normalize(direction);
}

glm::mat4 CreateSurfaceTransform(
    const Planet& planet,
    const glm::vec3& unitSurfaceDirection,
    float yawRadians,
    float uniformScale)
{
    const glm::vec3 radii = glm::max(
        glm::abs(planet.GetRenderScale()),
        glm::vec3(0.001f));
    const glm::vec3 localSurfacePosition = unitSurfaceDirection * radii;
    const glm::vec3 surfaceNormal = glm::normalize(
        localSurfacePosition / (radii * radii));
    const glm::vec3 referenceAxis = std::abs(surfaceNormal.z) < 0.95f
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(
        glm::cross(surfaceNormal, referenceAxis));
    const glm::vec3 bitangent = glm::normalize(
        glm::cross(tangent, surfaceNormal));

    glm::mat4 surfaceBasis(1.0f);
    surfaceBasis[0] = glm::vec4(tangent, 0.0f);
    surfaceBasis[1] = glm::vec4(surfaceNormal, 0.0f);
    surfaceBasis[2] = glm::vec4(bitangent, 0.0f);
    const glm::mat4 randomYaw = glm::rotate(
        glm::mat4(1.0f), yawRadians, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 worldPosition =
        planet.GetRenderPosition() + localSurfacePosition +
        surfaceNormal * 0.025f;
    return glm::translate(glm::mat4(1.0f), worldPosition) *
           surfaceBasis * randomYaw *
           glm::scale(glm::mat4(1.0f), glm::vec3(uniformScale));
}

void ConfigureInstanceAttributes(
    const LoadedModel& model,
    unsigned int instanceBuffer)
{
    for (const LoadedMesh& mesh : model.meshes) {
        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
        for (int column = 0; column < 4; ++column) {
            const unsigned int attributeIndex = 5 + column;
            glEnableVertexAttribArray(attributeIndex);
            glVertexAttribPointer(
                attributeIndex, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                reinterpret_cast<void*>(sizeof(glm::vec4) * column));
            glVertexAttribDivisor(attributeIndex, 1);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::uint32_t AppendHash(std::uint32_t hash, std::string_view text)
{
    constexpr std::uint32_t fnvPrime = 16777619u;
    for (const char character : text) {
        hash ^= static_cast<unsigned char>(character);
        hash *= fnvPrime;
    }
    return hash;
}

std::uint32_t CalculatePlanetSeed(const Planet& planet)
{
    std::uint32_t hash = 2166136261u;
    hash = AppendHash(hash, planet.GetModelPath());
    hash = AppendHash(hash, planet.GetTextureOverridePath());
    const glm::ivec3 position = glm::ivec3(
        glm::round(planet.GetRenderPosition() * 100.0f));
    return AppendHash(
        hash,
        std::to_string(position.x) + "," +
            std::to_string(position.y) + "," +
            std::to_string(position.z));
}

float CalculatePlanetRadius(const Planet& planet)
{
    const glm::vec3 scale = glm::abs(planet.GetRenderScale());
    return std::max({scale.x, scale.y, scale.z});
}

bool IsBatchNearPosition(
    const auto& batch,
    const glm::vec3& position,
    float additionalDistance)
{
    const float maximumDistance = batch.planetRadius +
        batch.maximumDrawDistance + additionalDistance;
    const glm::vec3 offset = batch.planetCenter - position;
    return glm::dot(offset, offset) <= maximumDistance * maximumDistance;
}
}

EnvironmentDecorationRenderer::EnvironmentDecorationRenderer(Game* game)
    : mGame(game)
{
}

EnvironmentDecorationRenderer::~EnvironmentDecorationRenderer() = default;

void EnvironmentDecorationRenderer::Prepare(const std::vector<Planet*>& planets)
{
    if (HavePlanetsChanged(planets)) {
        RebuildForPlanets(planets);
    }
}

void EnvironmentDecorationRenderer::Draw(
    Shader3D& shader,
    const glm::vec3& cameraPosition) const
{
    if (mBatches.empty()) {
        return;
    }

    glUniform1i(shader.GetLocUseInstancing(), 1);
    glUniform1i(shader.GetLocUseSkinning(), 0);
    glUniform1i(shader.GetLocUseBackTexture(), 0);
    glUniform1f(shader.GetLocEmissiveIntensity(), 0.0f);
    glUniform1f(shader.GetLocMaterialMinimumReflectance(), 0.0f);
    glUniform1f(shader.GetLocMaterialRimBoost(), 0.0f);
    glUniform2f(shader.GetLocTextureTiling(), 1.0f, 1.0f);
    glUniform3f(shader.GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);

    for (const DecorationBatch& batch : mBatches) {
        if (!batch.model || batch.transforms.empty() ||
            !IsBatchNearPosition(batch, cameraPosition, 0.0f)) {
            continue;
        }
        ConfigureInstanceAttributes(*batch.model, batch.instanceBuffer);
        for (const LoadedMesh& mesh : batch.model->meshes) {
            glBindVertexArray(mesh.VAO);
            if (mesh.textureID != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.textureID);
                glUniform1i(shader.GetLocDiffuseTexture(), 0);
                glUniform1i(shader.GetLocUseTexture(), 1);
            } else {
                glUniform1i(shader.GetLocUseTexture(), 0);
            }
            glUniform4f(
                shader.GetLocObjectColor(),
                mesh.diffuseColor[0], mesh.diffuseColor[1],
                mesh.diffuseColor[2], 1.0f);
            glDrawElementsInstanced(
                GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr,
                static_cast<GLsizei>(batch.transforms.size()));
        }
    }

    glUniform1i(shader.GetLocUseInstancing(), 0);
    glUniform1i(shader.GetLocUseTexture(), 0);
    glBindVertexArray(0);
}

void EnvironmentDecorationRenderer::DrawShadow(
    DirectionalShadowShader& shader,
    const glm::vec3& shadowFocusPosition) const
{
    if (mBatches.empty()) {
        return;
    }
    glUniform1i(shader.GetLocUseInstancing(), 1);
    glUniform1i(shader.GetLocUseSkinning(), 0);
    for (const DecorationBatch& batch : mBatches) {
        constexpr float shadowCasterMargin = 12.0f;
        if (!batch.model || batch.transforms.empty() ||
            !IsBatchNearPosition(
                batch, shadowFocusPosition, shadowCasterMargin)) {
            continue;
        }
        ConfigureInstanceAttributes(*batch.model, batch.instanceBuffer);
        for (const LoadedMesh& mesh : batch.model->meshes) {
            glBindVertexArray(mesh.VAO);
            glDrawElementsInstanced(
                GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr,
                static_cast<GLsizei>(batch.transforms.size()));
        }
    }
    glUniform1i(shader.GetLocUseInstancing(), 0);
    glBindVertexArray(0);
}

void EnvironmentDecorationRenderer::Shutdown()
{
    ReleaseInstanceBuffers();
    mDecoratedPlanets.clear();
}

bool EnvironmentDecorationRenderer::HavePlanetsChanged(
    const std::vector<Planet*>& planets) const
{
    std::vector<const Planet*> decoratedPlanets;
    for (const Planet* planet : planets) {
        if (planet && planet->GetIsActive() &&
            planet->GetVisualSettings().biome != Planet::Biome::None &&
            planet->GetVisualSettings().decorationDensity > 0.0f) {
            decoratedPlanets.push_back(planet);
        }
    }
    if (decoratedPlanets.size() != mDecoratedPlanets.size()) {
        return true;
    }

    for (std::size_t index = 0; index < decoratedPlanets.size(); ++index) {
        const Planet& planet = *decoratedPlanets[index];
        const DecoratedPlanetIdentity& identity = mDecoratedPlanets[index];
        const Planet::VisualSettings& settings = planet.GetVisualSettings();
        if (identity.planet != &planet ||
            identity.position != planet.GetRenderPosition() ||
            identity.scale != planet.GetRenderScale() ||
            identity.modelPath != planet.GetModelPath() ||
            identity.texturePath != planet.GetTextureOverridePath() ||
            identity.biome != settings.biome ||
            identity.decorationDensity != settings.decorationDensity) {
            return true;
        }
    }
    return false;
}

void EnvironmentDecorationRenderer::RebuildForPlanets(
    const std::vector<Planet*>& planets)
{
    ReleaseInstanceBuffers();
    mDecoratedPlanets.clear();
    if (!mGame || !mGame->GetMeshLoadSystem()) {
        return;
    }

    for (Planet* planet : planets) {
        if (!planet || !planet->GetIsActive()) {
            continue;
        }
        const Planet::VisualSettings& settings = planet->GetVisualSettings();
        if (settings.biome == Planet::Biome::None ||
            settings.decorationDensity <= 0.0f) {
            continue;
        }
        AppendPlanetBatches(*planet);
        mDecoratedPlanets.push_back({
            planet,
            planet->GetRenderPosition(),
            planet->GetRenderScale(),
            planet->GetModelPath(),
            planet->GetTextureOverridePath(),
            settings.biome,
            settings.decorationDensity,
        });
    }
}

void EnvironmentDecorationRenderer::AppendPlanetBatches(Planet& planet)
{
    const Planet::VisualSettings& visualSettings = planet.GetVisualSettings();
    const std::vector<glm::vec3> occupiedPositions =
        CollectOccupiedPositions(planet);
    std::mt19937 randomGenerator(CalculatePlanetSeed(planet));
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedDistribution(-1.0f, 1.0f);
    std::uniform_real_distribution<float> angleDistribution(
        0.0f, 6.28318530718f);

    const float planetRadius = CalculatePlanetRadius(planet);
    const float areaScale = glm::clamp(
        planetRadius * planetRadius / 64.0f, 0.45f, 2.0f);

    const auto appendDefinitions = [&](const auto& definitions) {
        for (const DecorationDefinition& definition : definitions) {
            DecorationBatch batch;
            batch.model = mGame->GetMeshLoadSystem()->ResolveLoadedModel(
                definition.modelPath);
            if (!batch.model || !batch.model->IsLoaded()) {
                continue;
            }
            batch.planetCenter = planet.GetRenderPosition();
            batch.planetRadius = planetRadius;
            batch.maximumDrawDistance = definition.maximumDrawDistance;

            std::vector<glm::vec3> clusterDirections;
            clusterDirections.reserve(definition.clusterCount);
            for (int index = 0; index < definition.clusterCount; ++index) {
                clusterDirections.push_back(CreateRandomUnitVector(
                    randomGenerator, signedDistribution));
            }

            const int targetInstanceCount = static_cast<int>(std::round(
                definition.baseInstanceCount * areaScale *
                visualSettings.decorationDensity));
            constexpr int maximumAttemptsPerInstance = 14;
            const int maximumAttempts =
                targetInstanceCount * maximumAttemptsPerInstance;
            for (int attempt = 0;
                 attempt < maximumAttempts &&
                 static_cast<int>(batch.transforms.size()) < targetInstanceCount;
                 ++attempt) {
                const std::size_t clusterIndex = static_cast<std::size_t>(
                    unitDistribution(randomGenerator) *
                    clusterDirections.size()) % clusterDirections.size();
                const glm::vec3 clusterOffset = CreateRandomUnitVector(
                    randomGenerator, signedDistribution) *
                    definition.clusterSpread *
                    std::pow(unitDistribution(randomGenerator), 1.8f);
                const glm::vec3 surfaceDirection = glm::normalize(
                    clusterDirections[clusterIndex] + clusterOffset);
                const float uniformScale = definition.minimumScale +
                    (definition.maximumScale - definition.minimumScale) *
                        unitDistribution(randomGenerator);
                const glm::mat4 transform = CreateSurfaceTransform(
                    planet, surfaceDirection,
                    angleDistribution(randomGenerator), uniformScale);
                if (!IsClearOfOccupiedPositions(
                        glm::vec3(transform[3]), occupiedPositions,
                        definition.obstacleClearance)) {
                    continue;
                }
                batch.transforms.push_back(transform);
            }
            UploadBatch(batch);
            if (!batch.transforms.empty()) {
                mBatches.push_back(std::move(batch));
            }
        }
    };

    if (visualSettings.biome == Planet::Biome::Grassland) {
        appendDefinitions(GrasslandDecorationDefinitions);
    } else if (visualSettings.biome == Planet::Biome::Rocky) {
        appendDefinitions(RockyDecorationDefinitions);
    }
}

void EnvironmentDecorationRenderer::ReleaseInstanceBuffers()
{
    for (DecorationBatch& batch : mBatches) {
        if (batch.instanceBuffer != 0) {
            glDeleteBuffers(1, &batch.instanceBuffer);
            batch.instanceBuffer = 0;
        }
    }
    mBatches.clear();
}

void EnvironmentDecorationRenderer::UploadBatch(DecorationBatch& batch)
{
    if (!batch.model || batch.transforms.empty()) {
        return;
    }
    glGenBuffers(1, &batch.instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, batch.instanceBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(batch.transforms.size() * sizeof(glm::mat4)),
        batch.transforms.data(),
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
