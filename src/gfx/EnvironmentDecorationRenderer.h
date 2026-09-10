#pragma once

#include "actor/Planet.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

class DirectionalShadowShader;
class Game;
struct LoadedModel;
class Shader3D;

class EnvironmentDecorationRenderer {
public:
    explicit EnvironmentDecorationRenderer(Game* game);
    ~EnvironmentDecorationRenderer();

    void Prepare(const std::vector<Planet*>& planets);
    void Draw(Shader3D& shader, const glm::vec3& cameraPosition) const;
    void DrawShadow(
        DirectionalShadowShader& shader,
        const glm::vec3& shadowFocusPosition) const;
    void Shutdown();

private:
    struct DecorationBatch {
        const LoadedModel* model = nullptr;
        unsigned int instanceBuffer = 0;
        std::vector<glm::mat4> transforms;
        glm::vec3 planetCenter{0.0f};
        float planetRadius = 0.0f;
        float maximumDrawDistance = 0.0f;
    };

    struct DecoratedPlanetIdentity {
        const Planet* planet = nullptr;
        glm::vec3 position{0.0f};
        glm::vec3 scale{0.0f};
        std::string modelPath;
        std::string texturePath;
        Planet::Biome biome = Planet::Biome::None;
        float decorationDensity = 0.0f;
    };

    bool HavePlanetsChanged(const std::vector<Planet*>& planets) const;
    void RebuildForPlanets(const std::vector<Planet*>& planets);
    void AppendPlanetBatches(Planet& planet);
    void ReleaseInstanceBuffers();
    void UploadBatch(DecorationBatch& batch);

    Game* mGame = nullptr;
    std::vector<DecoratedPlanetIdentity> mDecoratedPlanets;
    std::vector<DecorationBatch> mBatches;
};
