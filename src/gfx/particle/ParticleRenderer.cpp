#include "gfx/particle/ParticleRenderer.h"

#include "Game.h"
#include "gfx/particle/ParticleShader.h"
#include "system/ParticleSystem.h"
#include "thirdParty/stb_image.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <utility>

namespace {
constexpr float velocityEpsilonSquared = 0.000001f;

struct OpenGLState {
    GLboolean blendEnabled = GL_FALSE;
    GLboolean depthTestEnabled = GL_FALSE;
    GLboolean depthWriteEnabled = GL_TRUE;
    GLint blendSourceRgb = GL_ONE;
    GLint blendDestinationRgb = GL_ZERO;
    GLint blendSourceAlpha = GL_ONE;
    GLint blendDestinationAlpha = GL_ZERO;
    GLint currentProgram = 0;
    GLint currentVertexArray = 0;
    GLint activeTexture = GL_TEXTURE0;
    GLint boundTexture = 0;
    GLint arrayBuffer = 0;
};

OpenGLState CaptureOpenGLState()
{
    OpenGLState state;
    state.blendEnabled = glIsEnabled(GL_BLEND);
    state.depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &state.depthWriteEnabled);
    glGetIntegerv(GL_BLEND_SRC_RGB, &state.blendSourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &state.blendDestinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &state.blendSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &state.blendDestinationAlpha);
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.currentProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.currentVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state.activeTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.arrayBuffer);

    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.boundTexture);
    glActiveTexture(state.activeTexture);
    return state;
}

void RestoreOpenGLState(const OpenGLState& state)
{
    if (state.blendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    if (state.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(state.depthWriteEnabled);
    glBlendFuncSeparate(state.blendSourceRgb, state.blendDestinationRgb,
                        state.blendSourceAlpha, state.blendDestinationAlpha);

    glUseProgram(static_cast<GLuint>(state.currentProgram));
    glBindVertexArray(static_cast<GLuint>(state.currentVertexArray));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(state.arrayBuffer));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(state.boundTexture));
    glActiveTexture(state.activeTexture);
}
} // namespace

ParticleRenderer::ParticleRenderer(Game* game)
    : mGame(game),
      mShader(std::make_unique<ParticleShader>())
{
    InitializeBuffers();
}

ParticleRenderer::~ParticleRenderer()
{
    for (const auto& textureEntry : mTextureCache) {
        const GLuint texture = textureEntry.second;
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }

    if (mInstanceBuffer != 0) {
        glDeleteBuffers(1, &mInstanceBuffer);
    }
    if (mQuadBuffer != 0) {
        glDeleteBuffers(1, &mQuadBuffer);
    }
    if (mVertexArray != 0) {
        glDeleteVertexArrays(1, &mVertexArray);
    }
}

void ParticleRenderer::Draw(const glm::mat4& view, const glm::mat4& projection) const
{
    if (!mGame || !mShader || !mShader->GetShaderProgram() || mVertexArray == 0) {
        return;
    }

    const ParticleSystem* particleSystem = mGame->GetParticleSystem();
    if (!particleSystem || particleSystem->GetParticles().empty()) {
        return;
    }

    const glm::mat4 inverseView = glm::inverse(view);
    const glm::vec3 cameraPosition = glm::vec3(inverseView[3]);
    const glm::vec3 cameraRight = glm::normalize(glm::vec3(inverseView[0]));
    const glm::vec3 cameraUp = glm::normalize(glm::vec3(inverseView[1]));

    std::vector<RenderItem> additiveItems;
    std::vector<RenderItem> alphaItems;
    additiveItems.reserve(particleSystem->GetParticleCount());
    alphaItems.reserve(particleSystem->GetParticleCount());

    for (const Particle& particle : particleSystem->GetParticles()) {
        RenderItem renderItem;
        renderItem.particle = &particle;
        renderItem.instance = CreateInstanceData(particle, view);
        renderItem.cameraDistanceSquared = glm::dot(particle.position - cameraPosition, particle.position - cameraPosition);

        if (particle.blendMode == ParticleBlendMode::Alpha) {
            alphaItems.push_back(renderItem);
        } else {
            additiveItems.push_back(renderItem);
        }
    }

    const OpenGLState previousState = CaptureOpenGLState();

    glUseProgram(mShader->GetShaderProgram());
    glUniformMatrix4fv(mShader->GetLocView(), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(mShader->GetLocProj(), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(mShader->GetLocCameraRight(), 1, &cameraRight[0]);
    glUniform3fv(mShader->GetLocCameraUp(), 1, &cameraUp[0]);
    glUniform1i(mShader->GetLocDiffuseTexture(), 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBindVertexArray(mVertexArray);

    DrawAdditiveParticles(additiveItems);
    DrawAlphaParticles(std::move(alphaItems));

    RestoreOpenGLState(previousState);
}

std::size_t ParticleRenderer::BatchKeyHash::operator()(const BatchKey& key) const
{
    const std::size_t textureHash = std::hash<std::string>{}(key.texturePath);
    const std::size_t blendHash = std::hash<int>{}(static_cast<int>(key.blendMode));
    return textureHash ^ (blendHash + 0x9e3779b9U + (textureHash << 6U) + (textureHash >> 2U));
}

void ParticleRenderer::InitializeBuffers()
{
    // local position x/y, texture coordinate u/v
    constexpr float quadVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
    };

    glGenVertexArrays(1, &mVertexArray);
    glGenBuffers(1, &mQuadBuffer);
    glGenBuffers(1, &mInstanceBuffer);

    glBindVertexArray(mVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, mQuadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, mInstanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, position)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, size)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, color)));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, rotationRadians)));
    glVertexAttribDivisor(5, 1);

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                          reinterpret_cast<void*>(offsetof(InstanceData, stretch)));
    glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLuint ParticleRenderer::GetOrLoadTexture(const std::string& texturePath) const
{
    const auto textureIterator = mTextureCache.find(texturePath);
    if (textureIterator != mTextureCache.end()) {
        return textureIterator->second;
    }

    const GLuint texture = LoadTexture(texturePath);
    mTextureCache[texturePath] = texture;
    return texture;
}

GLuint ParticleRenderer::LoadTexture(const std::string& texturePath) const
{
    const std::string resolvedPath = ResolveTexturePath(texturePath);

    int width = 0;
    int height = 0;
    int channelCount = 0;

    stbi_set_flip_vertically_on_load(1);
    unsigned char* imageData = stbi_load(resolvedPath.c_str(), &width, &height, &channelCount, STBI_rgb_alpha);
    if (!imageData || width <= 0 || height <= 0) {
        std::cerr << "Failed to load particle texture: " << resolvedPath << '\n';
        stbi_image_free(imageData);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(imageData);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

ParticleRenderer::InstanceData ParticleRenderer::CreateInstanceData(const Particle& particle,
                                                                    const glm::mat4& view) const
{
    const float normalizedAge = particle.lifetime > 0.0f
                                    ? glm::clamp(particle.age / particle.lifetime, 0.0f, 1.0f)
                                    : 1.0f;

    InstanceData instance;
    instance.position = particle.position;
    instance.size = glm::mix(particle.startSize, particle.endSize, normalizedAge);
    instance.color = glm::mix(particle.startColor, particle.endColor, normalizedAge);
    instance.rotationRadians = particle.rotationRadians;
    instance.stretch = 1.0f;

    if (particle.renderMode == ParticleRenderMode::VelocityAligned &&
        glm::dot(particle.velocity, particle.velocity) > velocityEpsilonSquared) {
        const glm::vec3 viewVelocity = glm::vec3(view * glm::vec4(particle.velocity, 0.0f));
        if (viewVelocity.x * viewVelocity.x + viewVelocity.y * viewVelocity.y > velocityEpsilonSquared) {
            instance.rotationRadians += std::atan2(viewVelocity.y, viewVelocity.x);
        }

        instance.stretch += glm::length(particle.velocity) * particle.velocityStretch;
    }

    return instance;
}

void ParticleRenderer::DrawAdditiveParticles(const std::vector<RenderItem>& items) const
{
    if (items.empty()) {
        return;
    }

    std::unordered_map<BatchKey, std::vector<InstanceData>, BatchKeyHash> batches;
    for (const RenderItem& item : items) {
        if (!item.particle) {
            continue;
        }

        const BatchKey key{item.particle->texturePath, ParticleBlendMode::Additive};
        batches[key].push_back(item.instance);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const auto& batchEntry : batches) {
        DrawBatch(batchEntry.first, batchEntry.second);
    }
}

void ParticleRenderer::DrawAlphaParticles(std::vector<RenderItem> items) const
{
    if (items.empty()) {
        return;
    }

    std::sort(items.begin(), items.end(), [](const RenderItem& left, const RenderItem& right) {
        return left.cameraDistanceSquared > right.cameraDistanceSquared;
    });

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BatchKey currentKey;
    std::vector<InstanceData> currentInstances;
    bool hasCurrentBatch = false;

    const auto flushBatch = [&]() {
        if (hasCurrentBatch && !currentInstances.empty()) {
            DrawBatch(currentKey, currentInstances);
        }
        currentInstances.clear();
    };

    for (const RenderItem& item : items) {
        if (!item.particle) {
            continue;
        }

        const BatchKey itemKey{item.particle->texturePath, ParticleBlendMode::Alpha};
        if (!hasCurrentBatch || !(itemKey == currentKey)) {
            flushBatch();
            currentKey = itemKey;
            hasCurrentBatch = true;
        }

        currentInstances.push_back(item.instance);
    }

    flushBatch();
}

void ParticleRenderer::DrawBatch(const BatchKey& key, const std::vector<InstanceData>& instances) const
{
    if (instances.empty()) {
        return;
    }

    const GLuint texture = GetOrLoadTexture(key.texturePath);
    if (texture == 0) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, mInstanceBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)),
                 instances.data(),
                 GL_STREAM_DRAW);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(instances.size()));
}

std::string ParticleRenderer::ResolveTexturePath(const std::string& texturePath)
{
    if (texturePath.empty()) {
        return {};
    }

    if (texturePath.size() >= 2 && texturePath[0] == '.' && texturePath[1] == '.') {
        return texturePath;
    }

    return "../assets/textures/particles/" + texturePath;
}
