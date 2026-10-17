#pragma once

#include "gfx/Shader.h"

// 粒子のインスタンス描画に使うGLSLプログラムとuniform位置を保持する。
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
