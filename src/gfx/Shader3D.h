#pragma once

#include "Shader.h"

class Shader3D : public Shader {
public:
    Shader3D();
    ~Shader3D();

    void Initialize();

    int GetLocSunDirection() const { return mLocSunDirection; }
    int GetLocSunColor() const { return mLocSunColor; }
    int GetLocSunIntensity() const { return mLocSunIntensity; }
    int GetLocEnvironmentColor() const { return mLocEnvironmentColor; }
    int GetLocDayEnvironmentIntensity() const { return mLocDayEnvironmentIntensity; }
    int GetLocNightEnvironmentIntensity() const { return mLocNightEnvironmentIntensity; }
    int GetLocViewPos() const { return mLocViewPos; }
    int GetLocToonLevels() const { return mLocToonLevels; }
    int GetLocToonStrength() const { return mLocToonStrength; }
    int GetLocRimColor() const { return mLocRimColor; }
    int GetLocDayRimStrength() const { return mLocDayRimStrength; }
    int GetLocNightRimStrength() const { return mLocNightRimStrength; }
    int GetLocRimPower() const { return mLocRimPower; }
    int GetLocMaterialMinimumReflectance() const { return mLocMaterialMinimumReflectance; }
    int GetLocMaterialRimBoost() const { return mLocMaterialRimBoost; }
    int GetLocIsUnlit() const { return mLocIsUnlit; }
    int GetLocUseSkinning() const { return mLocUseSkinning; }
    int GetLocUseInstancing() const { return mLocUseInstancing; }
    int GetLocBoneTransforms() const { return mLocBoneTransforms; }
    int GetLocTextureTiling() const { return mLocTextureTiling; }
    int GetLocUseBackTexture() const { return mLocUseBackTexture; }
    int GetLocBackTexture() const { return mLocBackTexture; }
    int GetLocTextureSideBlendWidth() const
    {
        return mLocTextureSideBlendWidth;
    }
    int GetLocColorMultiplier() const { return mLocColorMultiplier; }
    int GetLocApplyOutputGamma() const { return mLocApplyOutputGamma; }
    int GetLocEmissiveColor() const { return mLocEmissiveColor; }
    int GetLocEmissiveIntensity() const { return mLocEmissiveIntensity; }
    int GetLocLightSpaceMatrix() const { return mLocLightSpaceMatrix; }
    int GetLocShadowMap() const { return mLocShadowMap; }
    int GetLocShadowsEnabled() const { return mLocShadowsEnabled; }
    int GetLocShadowMinimumBias() const { return mLocShadowMinimumBias; }
    int GetLocShadowMaximumBias() const { return mLocShadowMaximumBias; }

private:
    int mLocSunDirection;
    int mLocSunColor;
    int mLocSunIntensity;
    int mLocEnvironmentColor;
    int mLocDayEnvironmentIntensity;
    int mLocNightEnvironmentIntensity;
    int mLocViewPos;
    int mLocToonStrength;
    int mLocToonLevels;
    int mLocRimColor;
    int mLocDayRimStrength;
    int mLocNightRimStrength;
    int mLocRimPower;
    int mLocMaterialMinimumReflectance;
    int mLocMaterialRimBoost;
    int mLocIsUnlit;
    int mLocUseSkinning;
    int mLocUseInstancing;
    int mLocBoneTransforms;
    int mLocTextureTiling;
    int mLocUseBackTexture;
    int mLocBackTexture;
    int mLocTextureSideBlendWidth;
    int mLocColorMultiplier;
    int mLocApplyOutputGamma;
    int mLocEmissiveColor;
    int mLocEmissiveIntensity;
    int mLocLightSpaceMatrix;
    int mLocShadowMap;
    int mLocShadowsEnabled;
    int mLocShadowMinimumBias;
    int mLocShadowMaximumBias;
};
