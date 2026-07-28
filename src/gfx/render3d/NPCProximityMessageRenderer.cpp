#include "gfx/render3d/NPCProximityMessageRenderer.h"

#include <GL/glew.h>

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "gfx/Shader3D.h"
#include "gfx/VertexArray.h"
#include "utils/MathUtils.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

NPCProximityMessageRenderer::NPCProximityMessageRenderer(
    const Renderer3D* renderer)
    : mRenderer(renderer)
{
}

void NPCProximityMessageRenderer::Draw(
    const glm::mat4& viewMat,
    const std::vector<Planet*>& planets) const
{
    if (!mRenderer || !mRenderer->GetGame() ||
        !mRenderer->GetShader3D()) {
        return;
    }

    for (const Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (const NPC* npc : planet->GetNPCs()) {
            if (!npc || !npc->GetIsActive() ||
                !npc->ShouldShowProximityMessage()) {
                continue;
            }
            DrawMessage(viewMat, npc);
        }
    }
}

void NPCProximityMessageRenderer::DrawMessage(
    const glm::mat4& viewMat,
    const NPC* npc) const
{
    auto& renderer =
        *const_cast<Renderer3D*>(mRenderer);
    auto& vertexArrays = renderer.GetVertexArrays();
    auto& textures = renderer.GetTextures();

    const auto quadIt = vertexArrays.find("quad");
    const auto backgroundIt = textures.find("npcMessageBg");
    if (quadIt == vertexArrays.end() || backgroundIt == textures.end()) {
        return;
    }

    int textPixelWidth = 0;
    int textPixelHeight = 0;
    const SDL_Color textColor{35, 35, 42, 255};
    const GLuint textTexture = renderer.CreateTextTextureFor3D(
        npc->GetResolvedProximityMessageText(),
        textPixelWidth,
        textPixelHeight,
        textColor,
        1.0f);
    if (textTexture == 0 || textPixelWidth <= 0 || textPixelHeight <= 0) {
        return;
    }

    const float scale = npc->GetProximityMessageScale();
    const float textHeight = 0.28f * scale;
    const float textAspect =
        static_cast<float>(textPixelWidth) /
        static_cast<float>(textPixelHeight);
    const float textWidth = textHeight * textAspect;
    const float horizontalPadding = 0.42f * scale;
    const float backgroundWidth =
        std::max(1.35f * scale, textWidth + horizontalPadding * 2.0f);
    const float backgroundHeight = 0.68f * scale;
    const glm::vec3 center =
        npc->GetPos() +
        npc->GetUpVec() * npc->GetProximityMessageHeight();

    Shader3D* shader = renderer.GetShader3D();
    renderer.StartTransparentDraw();
    glDisable(GL_DEPTH_TEST);
    glUniform2f(shader->GetLocTextureTiling(), 1.0f, 1.0f);
    glUniform1i(shader->GetLocUseTexture(), 1);
    glUniform4f(
        shader->GetLocObjectColor(),
        1.0f,
        1.0f,
        1.0f,
        1.0f);
    quadIt->second->SetActive();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundIt->second);
    glUniform1i(shader->GetLocDiffuseTexture(), 0);
    const glm::mat4 backgroundBillboard =
        renderer.GetGame()->GetMathUtils()->CreateBillBoard(
            viewMat,
            center,
            npc->GetUpVec(),
            backgroundWidth,
            backgroundHeight);
    glUniformMatrix4fv(
        shader->GetLocModel(),
        1,
        GL_FALSE,
        glm::value_ptr(backgroundBillboard));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, textTexture);
    const glm::mat4 textBillboard =
        renderer.GetGame()->GetMathUtils()->CreateBillBoard(
            viewMat,
            center + npc->GetUpVec() * (0.04f * scale),
            npc->GetUpVec(),
            textWidth,
            textHeight);
    glUniformMatrix4fv(
        shader->GetLocModel(),
        1,
        GL_FALSE,
        glm::value_ptr(textBillboard));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUniform1i(shader->GetLocUseTexture(), 0);
    glEnable(GL_DEPTH_TEST);
    renderer.EndTransparentDraw();
    glDeleteTextures(1, &textTexture);
}
