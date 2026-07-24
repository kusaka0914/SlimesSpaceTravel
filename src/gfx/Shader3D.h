#pragma once

#include "Shader.h"

class Shader3D : public Shader {
public:
    Shader3D();
    ~Shader3D();

    void Initialize();

    int GetLocLightPos() const { return mLocLightPos; }
    int GetLocLightColor() const { return mLocLightColor; }
    int GetLocViewPos() const { return mLocViewPos; }
    int GetLocAmbientStrength() const { return mLocAmbientStrength; }
    int GetLocToonLevels() const { return mLocToonLevels; }
    int GetLocToonStrength() const { return mLocToonStrength; }
    int GetLocRimStrength() const { return mLocRimStrength; }
    int GetLocRimPower() const { return mLocRimPower; }
    int GetLocUseSkinning() const { return mLocUseSkinning; }
    int GetLocBoneTransforms() const { return mLocBoneTransforms; }
    int GetLocTextureTiling() const { return mLocTextureTiling; }

private:
    int mLocLightPos;
    int mLocLightColor;
    int mLocViewPos;
    int mLocAmbientStrength;
    int mLocToonStrength;
    int mLocToonLevels;
    int mLocRimStrength;
    int mLocRimPower;
    int mLocUseSkinning;
    int mLocBoneTransforms;
    int mLocTextureTiling;
};
