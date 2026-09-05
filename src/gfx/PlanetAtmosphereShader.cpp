#include "gfx/PlanetAtmosphereShader.h"

#include <GL/glew.h>

PlanetAtmosphereShader::PlanetAtmosphereShader()
{
    mShaderProgram = CreateShaderProgram(
        "../shaders/planetAtmosphereVertex.glsl",
        "../shaders/planetAtmosphereFragment.glsl");
    mLocModel = glGetUniformLocation(mShaderProgram, "model");
    mLocView = glGetUniformLocation(mShaderProgram, "view");
    mLocProj = glGetUniformLocation(mShaderProgram, "projection");
    mLocViewPosition =
        glGetUniformLocation(mShaderProgram, "viewPosition");
    mLocAtmosphereColor =
        glGetUniformLocation(mShaderProgram, "atmosphereColor");
    mLocAtmosphereStrength =
        glGetUniformLocation(mShaderProgram, "atmosphereStrength");
    mLocAtmospherePower =
        glGetUniformLocation(mShaderProgram, "atmospherePower");
    mLocSunDirection =
        glGetUniformLocation(mShaderProgram, "sunDirection");
}
