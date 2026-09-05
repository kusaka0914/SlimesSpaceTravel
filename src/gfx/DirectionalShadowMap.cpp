#include "gfx/DirectionalShadowMap.h"

#include "gfx/DirectionalShadowShader.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

DirectionalShadowMap::DirectionalShadowMap()
    : mShader(std::make_unique<DirectionalShadowShader>())
{
}

DirectionalShadowMap::~DirectionalShadowMap() = default;

bool DirectionalShadowMap::Begin(
    const glm::vec3& focusPosition,
    const glm::vec3& sunDirection)
{
    if (!mShader || mShader->GetShaderProgram() == 0 ||
        !EnsureDepthTarget()) {
        return false;
    }

    const float directionLength = glm::length(sunDirection);
    if (directionLength <= 0.0001f) {
        return false;
    }
    const glm::vec3 normalizedSunDirection =
        sunDirection / directionLength;
    const glm::vec3 lightPosition =
        focusPosition -
        normalizedSunDirection * mSettings.lightDistance;
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 lightViewUp =
        std::abs(glm::dot(normalizedSunDirection, worldUp)) > 0.95f
        ? glm::vec3(1.0f, 0.0f, 0.0f)
        : worldUp;
    const glm::mat4 lightView = glm::lookAt(
        lightPosition,
        focusPosition,
        lightViewUp);
    const float extent = mSettings.projectionHalfExtent;
    const glm::mat4 lightProjection = glm::ortho(
        -extent,
        extent,
        -extent,
        extent,
        mSettings.nearPlane,
        mSettings.farPlane);
    mLightSpaceMatrix = lightProjection * lightView;

    glGetIntegerv(GL_VIEWPORT, mPreviousViewport);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &mPreviousFramebuffer);
    glGetIntegerv(GL_CULL_FACE_MODE, &mPreviousCullFaceMode);
    GLboolean depthWriteEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
    mWasDepthWriteEnabled = depthWriteEnabled == GL_TRUE;
    mWasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    mWasBlendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
    mWasCullFaceEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glViewport(
        0,
        0,
        mSettings.mapResolution,
        mSettings.mapResolution);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(mShader->GetShaderProgram());
    glUniformMatrix4fv(
        mShader->GetLocLightSpaceMatrix(),
        1,
        GL_FALSE,
        glm::value_ptr(mLightSpaceMatrix));
    glUniform1i(mShader->GetLocUseSkinning(), 0);
    glUniform1i(mShader->GetLocUseInstancing(), 0);
    mIsPassActive = true;
    return true;
}

void DirectionalShadowMap::End()
{
    if (!mIsPassActive) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mPreviousFramebuffer);
    glViewport(
        mPreviousViewport[0],
        mPreviousViewport[1],
        mPreviousViewport[2],
        mPreviousViewport[3]);
    glDepthMask(mWasDepthWriteEnabled ? GL_TRUE : GL_FALSE);
    if (mWasDepthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if (mWasBlendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    if (mWasCullFaceEnabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(mPreviousCullFaceMode);
    } else {
        glDisable(GL_CULL_FACE);
    }
    mIsPassActive = false;
    mHasRenderedDepth = true;
}

void DirectionalShadowMap::Shutdown()
{
    if (mDepthTexture != 0) {
        glDeleteTextures(1, &mDepthTexture);
        mDepthTexture = 0;
    }
    if (mFramebuffer != 0) {
        glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
    }
    mShader.reset();
    mAllocatedResolution = 0;
    mHasRenderedDepth = false;
}

bool DirectionalShadowMap::EnsureDepthTarget()
{
    if (mFramebuffer != 0 && mDepthTexture != 0 &&
        mAllocatedResolution == mSettings.mapResolution) {
        return true;
    }

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);

    if (mDepthTexture != 0) {
        glDeleteTextures(1, &mDepthTexture);
        mDepthTexture = 0;
    }
    if (mFramebuffer != 0) {
        glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
    }

    glGenFramebuffers(1, &mFramebuffer);
    glGenTextures(1, &mDepthTexture);
    glBindTexture(GL_TEXTURE_2D, mDepthTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        mSettings.mapResolution,
        mSettings.mapResolution,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        mDepthTexture,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const bool isComplete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    if (!isComplete) {
        return false;
    }

    mAllocatedResolution = mSettings.mapResolution;
    return true;
}
