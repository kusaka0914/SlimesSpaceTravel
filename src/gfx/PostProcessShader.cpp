#include "gfx/PostProcessShader.h"

#include <GL/glew.h>

PostProcessShader::PostProcessShader()
{
    mShaderProgram = CreateShaderProgram(
        "../shaders/postProcessVertex.glsl",
        "../shaders/postProcessFragment.glsl");
    mLocPass = glGetUniformLocation(mShaderProgram, "renderPass");
    mLocSceneTexture = glGetUniformLocation(mShaderProgram, "sceneTexture");
    mLocBloomTexture = glGetUniformLocation(mShaderProgram, "bloomTexture");
    mLocHorizontalBlur = glGetUniformLocation(mShaderProgram, "horizontalBlur");
    mLocBloomThreshold = glGetUniformLocation(mShaderProgram, "bloomThreshold");
    mLocBloomSoftKnee = glGetUniformLocation(mShaderProgram, "bloomSoftKnee");
    mLocBloomStrength = glGetUniformLocation(mShaderProgram, "bloomStrength");
    mLocExposure = glGetUniformLocation(mShaderProgram, "exposure");
}
