#include "gfx/PlanetAtmosphereRenderer.h"

#include "actor/Planet.h"
#include "gfx/PlanetAtmosphereShader.h"
#include "system/mesh/LoadedMesh.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

PlanetAtmosphereRenderer::PlanetAtmosphereRenderer()
    : mShader(std::make_unique<PlanetAtmosphereShader>())
{
}

PlanetAtmosphereRenderer::~PlanetAtmosphereRenderer() = default;

void PlanetAtmosphereRenderer::Draw(
    const std::vector<Planet*>& planets,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    const glm::vec3& sunDirection) const
{
    if (!mShader || mShader->GetShaderProgram() == 0 || planets.empty()) {
        return;
    }

    const GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean wasBlendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean wasDepthWriteEnabled = GL_TRUE;
    GLint previousCullFaceMode = GL_BACK;
    GLint previousBlendSource = GL_SRC_ALPHA;
    GLint previousBlendDestination = GL_ONE_MINUS_SRC_ALPHA;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &wasDepthWriteEnabled);
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSource);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDestination);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(mShader->GetShaderProgram());
    glUniformMatrix4fv(
        mShader->GetLocView(),
        1,
        GL_FALSE,
        glm::value_ptr(view));
    glUniformMatrix4fv(
        mShader->GetLocProj(),
        1,
        GL_FALSE,
        glm::value_ptr(projection));
    glUniform3fv(
        mShader->GetLocViewPosition(),
        1,
        glm::value_ptr(cameraPosition));
    glUniform1f(
        mShader->GetLocAtmospherePower(),
        mSettings.fresnelPower);
    glUniform3fv(
        mShader->GetLocSunDirection(),
        1,
        glm::value_ptr(sunDirection));

    for (Planet* planet : planets) {
        if (!planet || !planet->GetIsActive()) {
            continue;
        }
        const glm::vec3 absoluteScale =
            glm::abs(planet->GetRenderScale());
        if (absoluteScale.x <= 0.001f ||
            absoluteScale.y <= 0.001f ||
            absoluteScale.z <= 0.001f) {
            continue;
        }
        const std::vector<LoadedMesh>* meshes = planet->GetMeshes();
        if (!meshes || meshes->empty()) {
            continue;
        }

        const Planet::VisualSettings& visualSettings =
            planet->GetVisualSettings();
        glUniform3fv(
            mShader->GetLocAtmosphereColor(),
            1,
            glm::value_ptr(visualSettings.atmosphereColor));
        glUniform1f(
            mShader->GetLocAtmosphereStrength(),
            visualSettings.atmosphereStrength);

        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), planet->GetRenderPosition()) *
            glm::scale(
                glm::mat4(1.0f),
                planet->GetRenderScale() * mSettings.shellScale);
        glUniformMatrix4fv(
            mShader->GetLocModel(),
            1,
            GL_FALSE,
            glm::value_ptr(model));
        for (const LoadedMesh& mesh : *meshes) {
            glBindVertexArray(mesh.VAO);
            glDrawElements(
                GL_TRIANGLES,
                mesh.indexCount,
                GL_UNSIGNED_INT,
                nullptr);
        }
    }

    glDepthMask(wasDepthWriteEnabled);
    glBlendFunc(previousBlendSource, previousBlendDestination);
    glCullFace(previousCullFaceMode);
    if (wasDepthTestEnabled == GL_FALSE) {
        glDisable(GL_DEPTH_TEST);
    }
    if (wasBlendEnabled == GL_FALSE) {
        glDisable(GL_BLEND);
    }
    if (wasCullFaceEnabled == GL_FALSE) {
        glDisable(GL_CULL_FACE);
    }
}

void PlanetAtmosphereRenderer::Shutdown()
{
    mShader.reset();
}
