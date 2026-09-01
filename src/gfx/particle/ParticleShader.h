#pragma once

#include "gfx/Shader.h"

class ParticleShader : public Shader {
public:
    ParticleShader();
    ~ParticleShader() = default;

    int GetLocCameraRight() const { return mLocCameraRight; }
    int GetLocCameraUp() const { return mLocCameraUp; }

private:
    void Initialize();

private:
    int mLocCameraRight = -1;
    int mLocCameraUp = -1;
};
