#include "gfx/render3d/DebugLabelRenderer.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "gfx/VertexArray.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"
#include "actor/StageObject.h"
#include "actor/TutorialTrigger.h"
#include "gfx/Shader3D.h"
#include "utils/MathUtils.h"

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

DebugLabelRenderer::DebugLabelRenderer(const Renderer3D* renderer)
    : mRenderer(renderer)
{
}

void DebugLabelRenderer::DrawDebugLabels(const glm::mat4& viewMat) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetGame()->GetCurrentStage()) {
        return;
    }

    const std::vector<Planet*>& planets = mRenderer->GetGame()->GetCurrentStage()->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            DrawDebugLabel(viewMat, platform, "足場 " + std::to_string(platform->GetStageYamlIndex()));
        }

        for (NPC* npc : planet->GetNPCs()) {
            DrawDebugLabel(viewMat, npc, "NPC " + std::to_string(npc->GetStageYamlIndex()));
        }

        for (TutorialTrigger* trigger :
             planet->GetTutorialTriggers()) {
            DrawDebugLabel(
                viewMat,
                trigger,
                "Tutorial Trigger " +
                    std::to_string(
                        trigger->GetStageYamlIndex()));
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            DrawDebugLabel(viewMat, enemy, "敵 " + std::to_string(enemy->GetStageYamlIndex()));
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            DrawDebugLabel(viewMat, crystal, "クリスタル " + std::to_string(crystal->GetStageYamlIndex()));
        }

        for (BoatParts* boatParts : planet->GetBoatParts()) {
            DrawDebugLabel(viewMat, boatParts, "ボートパーツ " + std::to_string(boatParts->GetStageYamlIndex()));
        }

        for (Boat* boat : planet->GetBoats()) {
            DrawDebugLabel(viewMat, boat, "ボート " + std::to_string(boat->GetStageYamlIndex()));
        }

        for (StageObject* stageObject : planet->GetStageObjects()) {
            DrawDebugLabel(
                viewMat,
                stageObject,
                stageObject->GetModelPath() + " " + std::to_string(stageObject->GetStageYamlIndex()));
        }

        if (planet->GetKey()) {
            DrawDebugLabel(viewMat, planet->GetKey(), "鍵 " + std::to_string(planet->GetKey()->GetStageYamlIndex()));
        }

        if (planet->GetStar()) {
            DrawDebugLabel(viewMat, planet->GetStar(), "スター " + std::to_string(planet->GetStar()->GetStageYamlIndex()));
        }
    }
}

void DebugLabelRenderer::DrawDebugLabel(const glm::mat4& viewMat, const Actor* actor, const std::string& label) const
{
    if (!mRenderer || !mRenderer->GetGame() || !mRenderer->GetShader3D() || !actor || !actor->GetIsActive()) {
        return;
    }

    int textWidth = 0;
    int textHeight = 0;

    const SDL_Color textColor{255, 255, 255, 255};
    GLuint textTexture = mRenderer->CreateTextTextureFor3D(label, textWidth, textHeight, textColor, 1.0f);

    if (textTexture == 0 || textWidth <= 0 || textHeight <= 0) {
        return;
    }

    auto& vertexArrays = const_cast<Renderer3D*>(mRenderer)->GetVertexArrays();
    auto quadIt = vertexArrays.find("quad");
    if (quadIt == vertexArrays.end()) {
        glDeleteTextures(1, &textTexture);
        return;
    }

    Shader3D* shader = mRenderer->GetShader3D();

    mRenderer->StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glUniform1i(shader->GetLocDiffuseTexture(), 0);
    glUniform1i(shader->GetLocUseTexture(), 1);
    glUniform4f(shader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    quadIt->second->SetActive();

    const float labelHeight = actor->GetRadius() * actor->GetScale().y + 0.8f;
    constexpr float baseHeight = 0.5f;
    const float aspect = static_cast<float>(textWidth) / static_cast<float>(textHeight);

    const float height = baseHeight;
    const float width = baseHeight * aspect;

    const glm::mat4 billboard =
        mRenderer->GetGame()->GetMathUtils()->CreateBillBoard(viewMat, actor, labelHeight, 0.0f, width, height);

    glUniformMatrix4fv(shader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUniform1i(shader->GetLocUseTexture(), 0);

    mRenderer->EndTransparentDraw();

    glDeleteTextures(1, &textTexture);
}
