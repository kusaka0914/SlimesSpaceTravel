#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"

#include "Game.h"
#include "gfx/Renderer3D.h"
#include "gfx/Shader3D.h"
#include "system/MeshLoadSystem.h"
#include "system/mesh/LoadedMesh.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
constexpr int ThumbnailResolution = 256;
constexpr float ThumbnailFieldOfViewDegrees = 35.0f;
constexpr float MinimumModelRadius = 0.001f;

class OpenGLStateGuard final {
public:
    OpenGLStateGuard()
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mFramebuffer);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &mRenderbuffer);
        glGetIntegerv(GL_CURRENT_PROGRAM, &mShaderProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &mVertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &mActiveTexture);
        glGetIntegerv(GL_VIEWPORT, mViewport);
        glGetIntegerv(GL_DEPTH_FUNC, &mDepthFunction);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, mClearColor);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &mDepthWriteEnabled);

        mWasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        mWasBlendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
        mWasCullFaceEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;

        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &mTextureUnitZeroBinding);
        glActiveTexture(mActiveTexture);
    }

    ~OpenGLStateGuard()
    {
        RestoreCapability(GL_DEPTH_TEST, mWasDepthTestEnabled);
        RestoreCapability(GL_BLEND, mWasBlendEnabled);
        RestoreCapability(GL_CULL_FACE, mWasCullFaceEnabled);
        glDepthMask(mDepthWriteEnabled);
        glDepthFunc(mDepthFunction);
        glClearColor(
            mClearColor[0],
            mClearColor[1],
            mClearColor[2],
            mClearColor[3]);
        glUseProgram(mShaderProgram);
        glBindVertexArray(mVertexArray);
        glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mRenderbuffer);
        glViewport(
            mViewport[0],
            mViewport[1],
            mViewport[2],
            mViewport[3]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            static_cast<GLuint>(mTextureUnitZeroBinding));
        glActiveTexture(mActiveTexture);
    }

private:
    static void RestoreCapability(GLenum capability, bool wasEnabled)
    {
        if (wasEnabled) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }

    GLint mFramebuffer = 0;
    GLint mRenderbuffer = 0;
    GLint mShaderProgram = 0;
    GLint mVertexArray = 0;
    GLint mActiveTexture = GL_TEXTURE0;
    GLint mTextureUnitZeroBinding = 0;
    GLint mViewport[4] = {};
    GLint mDepthFunction = GL_LESS;
    GLfloat mClearColor[4] = {};
    GLboolean mDepthWriteEnabled = GL_TRUE;
    bool mWasDepthTestEnabled = false;
    bool mWasBlendEnabled = false;
    bool mWasCullFaceEnabled = false;
};
}

EditorModelThumbnailRenderer::EditorModelThumbnailRenderer(Game* game)
    : mGame(game)
{
}

EditorModelThumbnailRenderer::~EditorModelThumbnailRenderer()
{
    Clear();
    if (mDepthBuffer != 0) {
        glDeleteRenderbuffers(1, &mDepthBuffer);
    }
    if (mFramebuffer != 0) {
        glDeleteFramebuffers(1, &mFramebuffer);
    }
}

void EditorModelThumbnailRenderer::BeginFrame()
{
    mDidGenerateThumbnailThisFrame = false;
}

GLuint EditorModelThumbnailRenderer::ResolveThumbnail(
    const std::string& modelPath)
{
    const auto cachedThumbnail = mThumbnailTextures.find(modelPath);
    if (cachedThumbnail != mThumbnailTextures.end()) {
        return cachedThumbnail->second;
    }
    if (modelPath.empty() ||
        mFailedModelPaths.contains(modelPath) ||
        mDidGenerateThumbnailThisFrame ||
        !mGame || !mGame->GetMeshLoadSystem()) {
        return 0;
    }

    mDidGenerateThumbnailThisFrame = true;
    const LoadedModel* loadedModel =
        mGame->GetMeshLoadSystem()->ResolveLoadedModel(modelPath);
    GLuint thumbnailTextureHandle = 0;
    if (!loadedModel || !loadedModel->IsLoaded() ||
        !loadedModel->hasBounds ||
        !GenerateThumbnail(
            *loadedModel,
            thumbnailTextureHandle)) {
        mFailedModelPaths.insert(modelPath);
        return 0;
    }

    mThumbnailTextures.emplace(modelPath, thumbnailTextureHandle);
    return thumbnailTextureHandle;
}

bool EditorModelThumbnailRenderer::HasFailed(
    const std::string& modelPath) const
{
    return mFailedModelPaths.contains(modelPath);
}

void EditorModelThumbnailRenderer::Clear()
{
    for (const auto& [modelPath, textureHandle] : mThumbnailTextures) {
        (void)modelPath;
        if (textureHandle != 0) {
            glDeleteTextures(1, &textureHandle);
        }
    }
    mThumbnailTextures.clear();
    mFailedModelPaths.clear();
    mDidGenerateThumbnailThisFrame = false;
}

bool EditorModelThumbnailRenderer::GenerateThumbnail(
    const LoadedModel& loadedModel,
    GLuint& generatedTextureHandle)
{
    if (!mGame || !mGame->GetRenderer3D() ||
        !EnsureFramebuffer()) {
        return false;
    }

    Shader3D* shader = mGame->GetRenderer3D()->GetShader3D();
    if (!shader || shader->GetShaderProgram() == 0) {
        return false;
    }

    OpenGLStateGuard stateGuard;

    glGenTextures(1, &generatedTextureHandle);
    if (generatedTextureHandle == 0) {
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, generatedTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        ThumbnailResolution,
        ThumbnailResolution,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        generatedTextureHandle,
        0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        glDeleteTextures(1, &generatedTextureHandle);
        generatedTextureHandle = 0;
        return false;
    }

    glViewport(0, 0, ThumbnailResolution, ThumbnailResolution);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClearColor(0.055f, 0.065f, 0.085f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::vec3 modelCenter =
        (loadedModel.boundsMinimum + loadedModel.boundsMaximum) * 0.5f;
    const glm::vec3 modelHalfExtents =
        (loadedModel.boundsMaximum - loadedModel.boundsMinimum) * 0.5f;
    const float modelRadius =
        std::max(glm::length(modelHalfExtents), MinimumModelRadius);
    const float fieldOfViewRadians =
        glm::radians(ThumbnailFieldOfViewDegrees);
    const float cameraDistance =
        modelRadius / std::sin(fieldOfViewRadians * 0.5f) * 1.1f;
    const glm::vec3 cameraDirection =
        glm::normalize(glm::vec3(1.0f, 0.65f, 1.0f));
    const glm::vec3 cameraPosition =
        modelCenter + cameraDirection * cameraDistance;
    const float nearPlane =
        std::max(0.001f, cameraDistance - modelRadius * 1.5f);
    const float farPlane =
        cameraDistance + modelRadius * 1.5f;

    const glm::mat4 modelMatrix(1.0f);
    const glm::mat4 viewMatrix = glm::lookAt(
        cameraPosition,
        modelCenter,
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projectionMatrix = glm::perspective(
        fieldOfViewRadians,
        1.0f,
        nearPlane,
        farPlane);

    glUseProgram(shader->GetShaderProgram());
    glUniformMatrix4fv(
        shader->GetLocModel(),
        1,
        GL_FALSE,
        glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(
        shader->GetLocView(),
        1,
        GL_FALSE,
        glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(
        shader->GetLocProj(),
        1,
        GL_FALSE,
        glm::value_ptr(projectionMatrix));
    glUniform3fv(
        shader->GetLocViewPos(),
        1,
        glm::value_ptr(cameraPosition));
    const glm::vec3 lightPosition =
        cameraPosition + glm::vec3(-modelRadius, modelRadius, 0.0f);
    glUniform3fv(
        shader->GetLocLightPos(),
        1,
        glm::value_ptr(lightPosition));
    glUniform3f(shader->GetLocLightColor(), 1.0f, 1.0f, 1.0f);
    glUniform1f(shader->GetLocAmbientStrength(), 0.55f);
    glUniform1f(shader->GetLocToonLevels(), 4.0f);
    glUniform1f(shader->GetLocToonStrength(), 0.35f);
    glUniform1f(shader->GetLocRimStrength(), 0.18f);
    glUniform1f(shader->GetLocRimPower(), 2.5f);
    glUniform1i(shader->GetLocUseSkinning(), 0);
    glUniform1i(shader->GetLocUseBackTexture(), 0);
    glUniform2f(shader->GetLocTextureTiling(), 1.0f, 1.0f);

    glActiveTexture(GL_TEXTURE0);
    for (const LoadedMesh& loadedMesh : loadedModel.meshes) {
        glBindVertexArray(loadedMesh.VAO);
        if (loadedMesh.textureID != 0) {
            glBindTexture(GL_TEXTURE_2D, loadedMesh.textureID);
            glUniform1i(shader->GetLocDiffuseTexture(), 0);
            glUniform1i(shader->GetLocUseTexture(), 1);
        } else {
            glUniform1i(shader->GetLocUseTexture(), 0);
        }
        glUniform4f(
            shader->GetLocObjectColor(),
            loadedMesh.diffuseColor[0],
            loadedMesh.diffuseColor[1],
            loadedMesh.diffuseColor[2],
            1.0f);
        glDrawElements(
            GL_TRIANGLES,
            loadedMesh.indexCount,
            GL_UNSIGNED_INT,
            nullptr);
    }

    return true;
}

bool EditorModelThumbnailRenderer::EnsureFramebuffer()
{
    if (mFramebuffer != 0 && mDepthBuffer != 0) {
        return true;
    }

    GLint previousFramebuffer = 0;
    GLint previousRenderbuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

    glGenFramebuffers(1, &mFramebuffer);
    glGenRenderbuffers(1, &mDepthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH_COMPONENT24,
        ThumbnailResolution,
        ThumbnailResolution);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        mDepthBuffer);

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        static_cast<GLuint>(previousFramebuffer));
    glBindRenderbuffer(
        GL_RENDERBUFFER,
        static_cast<GLuint>(previousRenderbuffer));
    return mFramebuffer != 0 && mDepthBuffer != 0;
}
