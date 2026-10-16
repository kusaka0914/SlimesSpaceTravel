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

    mLocView = glGetUniformLocation(mShaderProgram, "viewMatrix");
    mLocProj = glGetUniformLocation(mShaderProgram, "projectionMatrix");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "particleTexture");
    mLocCameraRight = glGetUniformLocation(mShaderProgram, "cameraRight");
    mLocCameraUp = glGetUniformLocation(mShaderProgram, "cameraUp");
}
