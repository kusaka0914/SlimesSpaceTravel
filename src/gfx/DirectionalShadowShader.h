#pragma once

#include "gfx/Shader.h"

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
