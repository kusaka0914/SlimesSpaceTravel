#include "gfx/particle/ParticleShader.h"

#include <GL/glew.h>

ParticleShader::ParticleShader()
    : Shader()
{
    Initialize();
}

void ParticleShader::Initialize()
{
    mShaderProgram = CreateShaderProgram("../shaders/particle.vert", "../shaders/particle.frag");
    if (!mShaderProgram) {
        return;
    }

    mLocView = glGetUniformLocation(mShaderProgram, "view");
    mLocProj = glGetUniformLocation(mShaderProgram, "projection");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "particleTexture");
    mLocCameraRight = glGetUniformLocation(mShaderProgram, "cameraRight");
    mLocCameraUp = glGetUniformLocation(mShaderProgram, "cameraUp");
}
