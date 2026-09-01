#pragma once

#include "effect/particle/ParticleTypes.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Game;
class ParticleShader;

class ParticleRenderer {
public:
    explicit ParticleRenderer(Game* game);
    ~ParticleRenderer();

    void Draw(const glm::mat4& view, const glm::mat4& projection) const;

private:
    struct InstanceData {
        glm::vec3 position{0.0f};
        float size = 0.0f;
        glm::vec4 color{1.0f};
        float rotationRadians = 0.0f;
        float stretch = 1.0f;
    };

    struct RenderItem {
        const Particle* particle = nullptr;
        InstanceData instance;
        float cameraDistanceSquared = 0.0f;
    };

    struct BatchKey {
        std::string texturePath;
        ParticleBlendMode blendMode = ParticleBlendMode::Additive;

        bool operator==(const BatchKey& other) const
        {
            return blendMode == other.blendMode && texturePath == other.texturePath;
        }
    };

    struct BatchKeyHash {
        std::size_t operator()(const BatchKey& key) const;
    };

    void InitializeBuffers();
    GLuint GetOrLoadTexture(const std::string& texturePath) const;
    GLuint LoadTexture(const std::string& texturePath) const;

    InstanceData CreateInstanceData(const Particle& particle, const glm::mat4& view) const;
    void DrawAdditiveParticles(const std::vector<RenderItem>& items) const;
    void DrawAlphaParticles(std::vector<RenderItem> items) const;
    void DrawBatch(const BatchKey& key, const std::vector<InstanceData>& instances) const;

    static std::string ResolveTexturePath(const std::string& texturePath);

private:
    Game* mGame = nullptr;
    std::unique_ptr<ParticleShader> mShader;

    GLuint mVertexArray = 0;
    GLuint mQuadBuffer = 0;
    GLuint mInstanceBuffer = 0;

    mutable std::unordered_map<std::string, GLuint> mTextureCache;
};
