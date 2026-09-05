#include <GL/glew.h>

#include "gfx/PostProcessRenderer.h"

#include "gfx/PostProcessShader.h"

#include <algorithm>

namespace {
constexpr int BrightExtractionPass = 0;
constexpr int BlurPass = 1;
constexpr int CompositePass = 2;
}

PostProcessRenderer::PostProcessRenderer()
    : mShader(std::make_unique<PostProcessShader>())
{
    constexpr float fullscreenVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
    };

    glGenVertexArrays(1, &mFullscreenVertexArray);
    glGenBuffers(1, &mFullscreenVertexBuffer);
    glBindVertexArray(mFullscreenVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mFullscreenVertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(fullscreenVertices),
        fullscreenVertices,
        GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

PostProcessRenderer::~PostProcessRenderer() = default;

bool PostProcessRenderer::BeginScene(int width, int height)
{
    mIsSceneActive = EnsureRenderTargets(width, height);
    if (!mIsSceneActive) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mSceneFramebuffer);
    glViewport(0, 0, width, height);
    return true;
}

void PostProcessRenderer::CompositeTo(
    unsigned int destinationFramebuffer,
    int width,
    int height)
{
    if (!mIsSceneActive || !mShader ||
        mShader->GetShaderProgram() == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, destinationFramebuffer);
        return;
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glUseProgram(mShader->GetShaderProgram());
    glBindVertexArray(mFullscreenVertexArray);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(mShader->GetLocSceneTexture(), 0);

    glBindFramebuffer(GL_FRAMEBUFFER, mBloomFramebuffers[0]);
    glViewport(0, 0, mBloomWidth, mBloomHeight);
    glUniform1i(mShader->GetLocPass(), BrightExtractionPass);
    glUniform1f(mShader->GetLocBloomThreshold(), mSettings.bloomThreshold);
    glUniform1f(mShader->GetLocBloomSoftKnee(), mSettings.bloomSoftKnee);
    glBindTexture(GL_TEXTURE_2D, mSceneTexture);
    DrawFullscreenQuad();

    const int blurIterationCount = std::max(0, mSettings.blurIterationCount);
    int sourceBloomTextureIndex = 0;
    for (int blurIteration = 0;
         blurIteration < blurIterationCount * 2;
         ++blurIteration) {
        const int destinationBloomTextureIndex =
            1 - sourceBloomTextureIndex;
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            mBloomFramebuffers[destinationBloomTextureIndex]);
        glUniform1i(mShader->GetLocPass(), BlurPass);
        glUniform1i(
            mShader->GetLocHorizontalBlur(),
            blurIteration % 2 == 0 ? 1 : 0);
        glBindTexture(
            GL_TEXTURE_2D,
            mBloomTextures[sourceBloomTextureIndex]);
        DrawFullscreenQuad();
        sourceBloomTextureIndex = destinationBloomTextureIndex;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, destinationFramebuffer);
    glViewport(0, 0, width, height);
    glUniform1i(mShader->GetLocPass(), CompositePass);
    glUniform1f(mShader->GetLocBloomStrength(), mSettings.bloomStrength);
    glUniform1f(mShader->GetLocExposure(), mSettings.exposure);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mSceneTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(
        GL_TEXTURE_2D,
        mBloomTextures[sourceBloomTextureIndex]);
    glUniform1i(mShader->GetLocBloomTexture(), 1);
    DrawFullscreenQuad();

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    if (wasDepthTestEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
    if (wasBlendEnabled == GL_TRUE) {
        glEnable(GL_BLEND);
    }
    if (wasCullFaceEnabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    }
    mIsSceneActive = false;
}

void PostProcessRenderer::Shutdown()
{
    DestroyRenderTargets();
    if (mFullscreenVertexBuffer != 0) {
        glDeleteBuffers(1, &mFullscreenVertexBuffer);
        mFullscreenVertexBuffer = 0;
    }
    if (mFullscreenVertexArray != 0) {
        glDeleteVertexArrays(1, &mFullscreenVertexArray);
        mFullscreenVertexArray = 0;
    }
    mShader.reset();
}

bool PostProcessRenderer::EnsureRenderTargets(int width, int height)
{
    if (width <= 0 || height <= 0 || !mShader ||
        mShader->GetShaderProgram() == 0) {
        return false;
    }
    if (width == mWidth && height == mHeight &&
        mSceneFramebuffer != 0 && mBloomFramebuffers[0] != 0) {
        return true;
    }

    DestroyRenderTargets();
    if (!CreateSceneTarget(width, height)) {
        DestroyRenderTargets();
        return false;
    }

    mBloomWidth = std::max(1, width / 2);
    mBloomHeight = std::max(1, height / 2);
    if (!CreateBloomTargets(mBloomWidth, mBloomHeight)) {
        DestroyRenderTargets();
        return false;
    }

    mWidth = width;
    mHeight = height;
    return true;
}

bool PostProcessRenderer::CreateSceneTarget(int width, int height)
{
    glGenFramebuffers(1, &mSceneFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mSceneFramebuffer);

    glGenTextures(1, &mSceneTexture);
    glBindTexture(GL_TEXTURE_2D, mSceneTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        width,
        height,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        mSceneTexture,
        0);

    glGenRenderbuffers(1, &mSceneDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, mSceneDepthBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        mSceneDepthBuffer);
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE;
}

bool PostProcessRenderer::CreateBloomTargets(int width, int height)
{
    glGenFramebuffers(2, mBloomFramebuffers);
    glGenTextures(2, mBloomTextures);
    for (int targetIndex = 0; targetIndex < 2; ++targetIndex) {
        glBindFramebuffer(GL_FRAMEBUFFER, mBloomFramebuffers[targetIndex]);
        glBindTexture(GL_TEXTURE_2D, mBloomTextures[targetIndex]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            width,
            height,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            mBloomTextures[targetIndex],
            0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            return false;
        }
    }
    return true;
}

void PostProcessRenderer::DrawFullscreenQuad() const
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PostProcessRenderer::DestroyRenderTargets()
{
    glDeleteRenderbuffers(1, &mSceneDepthBuffer);
    glDeleteTextures(1, &mSceneTexture);
    glDeleteFramebuffers(1, &mSceneFramebuffer);
    glDeleteTextures(2, mBloomTextures);
    glDeleteFramebuffers(2, mBloomFramebuffers);
    mSceneDepthBuffer = 0;
    mSceneTexture = 0;
    mSceneFramebuffer = 0;
    mBloomTextures[0] = 0;
    mBloomTextures[1] = 0;
    mBloomFramebuffers[0] = 0;
    mBloomFramebuffers[1] = 0;
    mWidth = 0;
    mHeight = 0;
    mBloomWidth = 0;
    mBloomHeight = 0;
    mIsSceneActive = false;
}
