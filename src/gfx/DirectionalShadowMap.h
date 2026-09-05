#pragma once

#include "gfx/LightingSettings.h"

#include <glm/glm.hpp>

#include <memory>

class DirectionalShadowShader;

class DirectionalShadowMap {
public:
    DirectionalShadowMap();
    ~DirectionalShadowMap();

    bool Begin(
        const glm::vec3& focusPosition,
        const glm::vec3& sunDirection);
    void End();
    void Shutdown();

    DirectionalShadowShader* GetShader() const { return mShader.get(); }
    unsigned int GetDepthTexture() const { return mDepthTexture; }
    const glm::mat4& GetLightSpaceMatrix() const
    {
        return mLightSpaceMatrix;
    }
    const DirectionalShadowSettings& GetSettings() const
    {
        return mSettings;
    }
    bool IsAvailable() const
    {
        return mDepthTexture != 0 && mHasRenderedDepth;
    }

private:
    bool EnsureDepthTarget();

    std::unique_ptr<DirectionalShadowShader> mShader;
    DirectionalShadowSettings mSettings;
    unsigned int mFramebuffer = 0;
    unsigned int mDepthTexture = 0;
    int mAllocatedResolution = 0;
    glm::mat4 mLightSpaceMatrix{1.0f};
    int mPreviousViewport[4]{};
    int mPreviousFramebuffer = 0;
    int mPreviousCullFaceMode = 0;
    bool mWasDepthTestEnabled = false;
    bool mWasBlendEnabled = false;
    bool mWasCullFaceEnabled = false;
    bool mWasDepthWriteEnabled = true;
    bool mIsPassActive = false;
    bool mHasRenderedDepth = false;
};
