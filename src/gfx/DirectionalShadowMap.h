#pragma once

#include "gfx/LightingSettings.h"

#include <glm/glm.hpp>
#include <memory>

class DirectionalShadowShader;

// 注目位置を中心とする平行光源の深度パスと、その描画先を管理する。
class DirectionalShadowMap {
public:
    DirectionalShadowMap();
    ~DirectionalShadowMap();

    // ワールド座標の注目位置・光の進行方向から深度パスを開始する。成功時はtrueを返し、Endと対にして使う。
    bool Begin(const glm::vec3& focusPosition, const glm::vec3& sunDirection);

    // 保存した描画先・viewport・深度書き込みと、深度テスト・合成・カリング・シザーの有効状態を戻す。
    // 未開始なら何もしない。
    void End();

    // OpenGLコンテキストが有効な間に呼び、深度描画先とシェーダーを解放する。
    void Shutdown();

    DirectionalShadowShader* GetShader() const { return mShader.get(); }
    unsigned int GetDepthTexture() const { return mDepthTexture; }
    const glm::mat4& GetLightSpaceMatrix() const { return mLightSpaceMatrix; }
    const DirectionalShadowSettings& GetSettings() const { return mSettings; }

    // 深度パスが少なくとも1回完了し、深度テクスチャが存在する場合にtrue。
    bool IsAvailable() const { return mDepthTexture != 0 && mHasRenderedDepth; }

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
    bool mWasScissorTestEnabled = false;
    bool mWasDepthWriteEnabled = true;
    bool mIsPassActive = false;
    bool mHasRenderedDepth = false;
};
