#pragma once

#include "gfx/Shader.h"

// 深度描画用GLSLプログラムを生成し、通常・スキニング・インスタンス描画のuniform位置を保持する。
class DirectionalShadowShader final : public Shader {
public:
    DirectionalShadowShader();

    int GetLocLightSpaceMatrix() const { return mLocLightSpaceMatrix; }
    int GetLocUseSkinning() const { return mLocUseSkinning; }
    int GetLocUseInstancing() const { return mLocUseInstancing; }
    int GetLocBoneTransforms() const { return mLocBoneTransforms; }

private:
    int mLocLightSpaceMatrix = -1;
    int mLocUseSkinning = -1;
    int mLocUseInstancing = -1;
    int mLocBoneTransforms = -1;
};
