#pragma once

#include "effect/particle/ParticleTypes.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class ParticleShader;

// ParticleSystemの粒子をビルボードで描画し、シェーダー・バッファ・テクスチャを所有する。
// 生成・描画・破棄は有効なGLコンテキスト上で行う。入力の粒子は描画中だけ借用する。
class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    // 加算粒子をバッチ描画し、半透明粒子を奥から描く。使用した主要なGL描画状態は復元する。
    void Draw(const glm::mat4& view, const glm::mat4& projection, std::span<const Particle> particles) const;

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
    std::unique_ptr<ParticleShader> mShader;

    GLuint mVertexArray = 0;
    GLuint mQuadBuffer = 0;
    GLuint mInstanceBuffer = 0;

    mutable std::unordered_map<std::string, GLuint> mTextureCache;
};
