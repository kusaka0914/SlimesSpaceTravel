#pragma once

#include "gfx/Shader.h"

class PostProcessShader : public Shader {
public:
    PostProcessShader();

    int GetLocPass() const { return mLocPass; }
    int GetLocSceneTexture() const { return mLocSceneTexture; }
    int GetLocBloomTexture() const { return mLocBloomTexture; }
    int GetLocHorizontalBlur() const { return mLocHorizontalBlur; }
    int GetLocBloomThreshold() const { return mLocBloomThreshold; }
    int GetLocBloomSoftKnee() const { return mLocBloomSoftKnee; }
    int GetLocBloomStrength() const { return mLocBloomStrength; }
    int GetLocExposure() const { return mLocExposure; }

private:
    int mLocPass = -1;
    int mLocSceneTexture = -1;
    int mLocBloomTexture = -1;
    int mLocHorizontalBlur = -1;
    int mLocBloomThreshold = -1;
    int mLocBloomSoftKnee = -1;
    int mLocBloomStrength = -1;
    int mLocExposure = -1;
};
