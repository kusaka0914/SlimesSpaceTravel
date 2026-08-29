#include "gfx/render3d/NPCProximityMessageRenderer.h"

#include <GL/glew.h>

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "gfx/Shader3D.h"
#include "gfx/UIRenderer.h"
#include "gfx/VertexArray.h"
#include "system/UILoadSystem.h"
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
    const Renderer3D& renderer = *mRenderer;
    VertexArray* quad = renderer.FindVertexArray("quad");
    const GLuint backgroundTexture =
        renderer.FindTexture("npcMessageBg");
    if (!quad || backgroundTexture == 0) {
        return;
    }

    int textPixelWidth = 0;
    int textPixelHeight = 0;
    int baseTextPixelHeight = 0;
    const SDL_Color textColor{35, 35, 42, 255};
    float rubyScaleRatio = 0.36f;
    float rubyGapRatio = -0.46f;
    if (UIRenderer* uiRenderer =
            renderer.GetGame()->GetUIRenderer()) {
        if (UILoadSystem* uiLoadSystem =
                uiRenderer->GetUILoadSystem()) {
            if (const UILoadSystem::TextInfo* talkTextInfo =
                    uiLoadSystem->GetTextInfo(
                        "state",
                        "talkText")) {
                rubyScaleRatio =
                    talkTextInfo->rubyScaleRatio;
                rubyGapRatio =
                    talkTextInfo->rubyGapRatio;
            }
        }
    }

    GLuint textTexture = 0;
    const std::vector<RubyTextSegment>& rubySegments =
        npc->GetResolvedProximityMessageRubySegments();
    if (!rubySegments.empty()) {
        textTexture =
            renderer.CreateRubyTextTextureFor3D(
                rubySegments,
                textPixelWidth,
                textPixelHeight,
                baseTextPixelHeight,
                textColor,
                rubyScaleRatio,
                rubyGapRatio);
    } else {
        textTexture = renderer.CreateTextTextureFor3D(
            npc->GetResolvedProximityMessageText(),
            textPixelWidth,
            textPixelHeight,
            textColor,
            1.0f);
        baseTextPixelHeight = textPixelHeight;
    }

    if (textTexture == 0 ||
        textPixelWidth <= 0 ||
        textPixelHeight <= 0 ||
        baseTextPixelHeight <= 0) {
        return;
    }

    const float scale = npc->GetProximityMessageScale();
    const float baseTextHeight = 0.28f * scale;
    const float pixelToWorldScale =
        baseTextHeight /
        static_cast<float>(baseTextPixelHeight);
    const float textWidth =
        static_cast<float>(textPixelWidth) *
        pixelToWorldScale;
    const float textHeight =
        static_cast<float>(textPixelHeight) *
        pixelToWorldScale;
    const float horizontalPadding = 0.42f * scale;
    const float backgroundWidth =
        std::max(1.35f * scale, textWidth + horizontalPadding * 2.0f);
    const float backgroundHeight =
        std::max(
            0.68f * scale,
            textHeight + 0.3f * scale);
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
    quad->SetActive();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
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
