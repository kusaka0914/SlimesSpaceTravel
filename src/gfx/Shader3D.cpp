#include "gfx/Shader3D.h"

#include <GL/glew.h>

Shader3D::Shader3D()
    : Shader()
{
    Initialize();
}

Shader3D::~Shader3D() = default;

void Shader3D::Initialize()
{
    mShaderProgram = CreateShaderProgram("../shaders/vertex.glsl", "../shaders/fragment.glsl");
    mLocModel = glGetUniformLocation(mShaderProgram, "model");
    mLocView = glGetUniformLocation(mShaderProgram, "view");
    mLocProj = glGetUniformLocation(mShaderProgram, "projection");
    mLocObjectColor = glGetUniformLocation(mShaderProgram, "objectColor");
    mLocUseTexture = glGetUniformLocation(mShaderProgram, "useTexture");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "diffuseTexture");
    mLocLightPos = glGetUniformLocation(mShaderProgram, "lightPos");
    mLocLightColor = glGetUniformLocation(mShaderProgram, "lightColor");
    mLocViewPos = glGetUniformLocation(mShaderProgram, "viewPos");
    mLocAmbientStrength = glGetUniformLocation(mShaderProgram, "ambientStrength");
    mLocToonLevels = glGetUniformLocation(mShaderProgram, "toonLevels");
    mLocToonStrength = glGetUniformLocation(mShaderProgram, "toonStrength");
    mLocRimStrength = glGetUniformLocation(mShaderProgram, "rimStrength");
    mLocRimPower = glGetUniformLocation(mShaderProgram, "rimPower");
    mLocUseSkinning = glGetUniformLocation(mShaderProgram, "useSkinning");
    mLocBoneTransforms = glGetUniformLocation(mShaderProgram, "boneTransforms[0]");
    mLocTextureTiling = glGetUniformLocation(mShaderProgram, "textureTiling");
}
