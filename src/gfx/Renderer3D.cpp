#include "Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "VertexArray.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "gfx/Shader3D.h"
#include "system/CameraSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"
#include <SDL_ttf.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>

Renderer3D::Renderer3D(Game* game)
    : Renderer(game),
      mAttackRangeVAO(0),
      mAttackRangeVBO(0)
{
    Initialize();
}

Renderer3D::~Renderer3D() = default;

void Renderer3D::Initialize()
{
    mShader3DUnique = std::make_unique<Shader3D>();
    mShader3D = mShader3DUnique.get();

    if (!mShader3D->GetShaderProgram()) {
        glfwTerminate();
        return;
    }

    std::string basePath = "../assets/textures/";
    RegisterTexture(basePath + "guard.png", "guard");
    RegisterTexture(basePath + "tired_star.png", "tired_star");

    glGenVertexArrays(1, &mAttackRangeVAO);
    glGenBuffers(1, &mAttackRangeVBO);

    glBindVertexArray(mAttackRangeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mAttackRangeVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer3D::Draw() const
{
    const bool isTitle = mGame->GetSceneSystem()->IsTitle();
    if (isTitle) {
        return;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(mGame->GetWindow(), &fbWidth, &fbHeight);

    glUseProgram(mShader3D->GetShaderProgram());

    if (!mGame->GetIsPlayer2Joined()) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight);
        return;
    }

    DrawGameScreenForMultiPerson(fbWidth, fbHeight);
}

void Renderer3D::DrawGameScreenForSinglePerson(float fbWidth, float fbHeight) const
{
    glViewport(0, 0, fbWidth, fbHeight);

    float aspect = fbWidth / fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    glm::mat4 view = mGame->GetCameraSystem()->GetViews()[0];
    glm::vec3 cameraPos = mGame->GetCameraSystem()->GetPlayerCameraPos(0);

    DrawScene(view, proj, cameraPos);
}

void Renderer3D::DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const
{
    std::vector<glm::mat4> views = mGame->GetCameraSystem()->GetViews();

    if (views.size() < 2) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight);
        return;
    }

    const float halfHeight = fbHeight * 0.5f;
    const float aspect = fbWidth / halfHeight;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    const glm::vec3 p1CameraPos = mGame->GetCameraSystem()->GetPlayerCameraPos(0);
    const glm::vec3 p2CameraPos = mGame->GetCameraSystem()->GetPlayerCameraPos(1);

    glViewport(0, static_cast<GLint>(halfHeight), static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    DrawScene(views[0], proj, p1CameraPos);

    glViewport(0, 0, static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    DrawScene(views[1], proj, p2CameraPos);
}

// void Renderer3D::DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const
// {
//     float halfWidth = fbWidth * 0.5f;
//     float halfAspect = halfWidth / static_cast<float>(fbHeight);
//     glm::mat4 halfProj = glm::perspective(glm::radians(45.0f), halfAspect, 0.1f, 100.0f);
//     std::vector<glm::mat4> views = mGame->GetCameraSystem()->GetViews();

//     glViewport(0, 0, static_cast<GLsizei>(halfWidth), fbHeight);
//     DrawScene(views[0], halfProj);

//     glViewport(static_cast<GLsizei>(halfWidth), 0, static_cast<GLsizei>(halfWidth), fbHeight);
//     DrawScene(views[1], halfProj);
// }

void Renderer3D::DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos) const
{
    SetUniforms(viewMat, projMat, cameraPos);

    std::vector<Planet*> planets = mGame->GetCurrentStage()->GetPlanets();

    glUniform1f(mShader3D->GetLocToonLevels(), 5.0f);
    glUniform1f(mShader3D->GetLocToonStrength(), 0.45f);
    TryDrawActors(planets, false);

    glUniform1f(mShader3D->GetLocToonLevels(), 3.0f);
    glUniform1f(mShader3D->GetLocToonStrength(), 0.6f);
    TryDrawActorOnPlanets(planets, viewMat);
    TryDrawPlayers(viewMat);

    if (mGame->GetIsDebugEditorShowing()) {
        DrawDebugLabels(viewMat);
    }
}

void Renderer3D::SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos) const
{
    glUniformMatrix4fv(mShader3D->GetLocView(), 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(mShader3D->GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));

    glUniform3f(mShader3D->GetLocViewPos(), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(mShader3D->GetLocLightPos(), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(mShader3D->GetLocLightColor(), 0.5f, 0.5f, 0.5f);

    glUniform1f(mShader3D->GetLocAmbientStrength(), 0.8f);
    glUniform1f(mShader3D->GetLocRimStrength(), 0.20f);
    glUniform1f(mShader3D->GetLocRimPower(), 2.5f);
}

void Renderer3D::TryDrawActorOnPlanets(const std::vector<Planet*>& planets, glm::mat4 viewMat) const
{
    for (auto planet : planets) {
        TryDrawEnemies(planet->GetEnemies(), viewMat);
        TryDrawActors(planet->GetBoats());
        TryDrawActors(planet->GetBoatParts());
        TryDrawActors(planet->GetCrystals());
        TryDrawActors(planet->GetPlatforms());
        TryDrawActors(planet->GetNPCs());
        TryDrawActor(planet->GetKey());
        TryDrawActor(planet->GetStar());
    }
}

void Renderer3D::TryDrawPlayers(const glm::mat4& viewMat) const
{
    std::vector<Player*> players = mGame->GetPlayers();
    TryDrawActor(players[0]);

    bool canDrawAttackRange =
        players[0]->IsAttacking() || players[0]->GetIsStrongAttacked() || players[0]->GetCanSpecialAttack();
    if (canDrawAttackRange) {
        DrawAttackRange(players[0]);
    }

    DrawTiredEffect(viewMat, players[0]);

    if (mGame->GetIsPlayer2Joined()) {
        TryDrawActor(players[1]);
        canDrawAttackRange =
            players[1]->IsAttacking() || players[1]->GetIsStrongAttacked() || players[1]->GetCanSpecialAttack();
        if (canDrawAttackRange) {
            DrawAttackRange(players[1]);
        }
        DrawTiredEffect(viewMat, players[1]);
    }
}

void Renderer3D::DrawTiredEffect(const glm::mat4& viewMat, const Player* player) const
{
    if (!player || !player->GetIsActive() || !player->GetIsTired()) {
        return;
    }

    auto texIt = mTextures.find("tired_star");
    if (texIt == mTextures.end()) {
        return;
    }

    StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texIt->second);
    glUniform1i(mShader3D->GetLocDiffuseTexture(), 0);
    glUniform1i(mShader3D->GetLocUseTexture(), 1);
    glUniform4f(mShader3D->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    mVertexArrays.at("quad")->SetActive();

    constexpr int starCount = 3;
    constexpr float orbitRadius = 0.35f;
    constexpr float headHeight = 1.35f;
    constexpr float starSize = 0.28f;
    constexpr float rotateSpeed = 4.5f;

    const float time = static_cast<float>(glfwGetTime());

    glm::vec3 up = player->GetUpVec();
    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    glm::vec3 forward = player->GetFacingForwardVec();
    forward = forward - up * glm::dot(forward, up);

    if (glm::length(forward) < 1e-6f) {
        forward = player->GetForwardVec();
        forward = forward - up * glm::dot(forward, up);
    }

    if (glm::length(forward) < 1e-6f) {
        forward = glm::vec3(0.0f, 0.0f, 1.0f);
        forward = forward - up * glm::dot(forward, up);
    }

    forward = glm::normalize(forward);

    glm::vec3 left = glm::normalize(glm::cross(up, forward));
    glm::vec3 center = player->GetPos() + up * headHeight;

    for (int i = 0; i < starCount; ++i) {
        const float angle =
            time * rotateSpeed + glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(starCount);

        const glm::vec3 orbitOffset = forward * std::cos(angle) * orbitRadius + left * std::sin(angle) * orbitRadius;

        const float scale = starSize * (1.0f + 0.15f * std::sin(time * 8.0f + static_cast<float>(i)));

        glm::mat4 billboard = mGame->GetMathUtils()->CreateBillBoard(viewMat, center + orbitOffset, up, scale, scale);

        glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    EndTransparentDraw();
}

void Renderer3D::TryDrawEnemies(const std::vector<Enemy*>& enemies, const glm::mat4& viewMat) const
{
    if (enemies.empty()) {
        return;
    }

    for (Enemy* enemy : enemies) {
        if (!enemy->GetIsActive()) {
            continue;
        }

        DrawActor(enemy, true);
        DrawEnemyGuard(viewMat, enemy);
        DrawEnemyHp(viewMat, enemy);

        if (enemy->GetStandByAttackTimer() > 0.0f && enemy->GetStandByAttackTimer() <= 1.0f) {
            DrawEnemyAttackRange(enemy);
        }
    }
}

void Renderer3D::TryDrawActor(Actor* actor, bool useOrient) const
{
    if (!actor) {
        return;
    }

    if (actor->GetIsActive()) {
        DrawActor(actor, useOrient);
    }
}

void Renderer3D::DrawActor(Actor* actor, bool useOrient) const
{
    if (!actor) {
        return;
    }

    if (actor->GetIsEditorSelected()) {
        DrawActorSelectionOutline(actor, useOrient);
    }

    glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    GLint locObjectColor = mShader3D->GetLocObjectColor();
    GLint locUseTexture = mShader3D->GetLocUseTexture();

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        return;
    }

    for (auto actorMesh : *actorMeshes) {
        glBindVertexArray(actorMesh.VAO);
        if (actorMesh.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, actorMesh.textureID);
            glUniform1i(mShader3D->GetLocDiffuseTexture(), 0);
            glUniform1i(locUseTexture, 1);
        } else {
            glUniform1i(locUseTexture, 0);
        }
        glUniform4f(locObjectColor, actorMesh.diffuseColor[0], actorMesh.diffuseColor[1], actorMesh.diffuseColor[2],
                    1.0f);
        glDrawElements(GL_TRIANGLES, actorMesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glUniform1i(locUseTexture, 0);
    return;
}

void Renderer3D::DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const
{
    const int breakCount = enemy->GetBreakCount();
    if (breakCount == 0) {
        return;
    }

    StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextures.at("guard"));
    GLint locUseTexture = mShader3D->GetLocUseTexture();
    glUniform1i(locUseTexture, 1);
    mVertexArrays.at("quad")->SetActive();

    const float upMargin = enemy->GetRadius() * 0.8f;
    constexpr float guardWidth = 0.5f;
    constexpr float guardHeight = 0.5f;
    for (int i = 0; i < breakCount; i++) {
        const float rightMargin = (i - (breakCount - 1) * 0.5f) * 0.4f;
        glm::mat4 billboard =
            mGame->GetMathUtils()->CreateBillBoard(viewMat, enemy, upMargin, rightMargin, guardWidth, guardHeight);
        glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1i(locUseTexture, 0);
    EndTransparentDraw();
}

void Renderer3D::DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const
{
    StartTransparentDraw();
    mVertexArrays.at("hpBar")->SetActive();

    constexpr float rightMargin = -0.5f;
    const float upMargin = enemy->GetRadius() * 1.5f;
    const float hpWidth = enemy->GetHp() / enemy->GetMaxHp();
    constexpr float hpHeight = 0.1f;

    glm::mat4 billboard =
        mGame->GetMathUtils()->CreateBillBoard(viewMat, enemy, upMargin, rightMargin, hpWidth, hpHeight);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));

    std::vector<GLfloat> hpGreen{0.0f, 1.0f, 0.0f, 1.0f};
    glUniform4fv(mShader3D->GetLocObjectColor(), 1, hpGreen.data());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    EndTransparentDraw();
}

void Renderer3D::StartTransparentDraw() const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
}

void Renderer3D::EndTransparentDraw() const
{
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer3D::DrawAttackRange(Player* player) const
{
    if (!player) {
        return;
    }

    constexpr int segments = 48;

    const float attackRange = player->GetAttackRange();
    const float attackAngle = player->GetAttackAngle();

    if (attackRange <= 0.0f || attackAngle <= 0.0f) {
        return;
    }

    const glm::vec3 center = player->GetPos();
    const glm::vec3 up = glm::normalize(player->GetUpVec());
    const glm::vec3 forward = glm::normalize(player->GetFacingForwardVec());
    const glm::vec3 left = glm::normalize(player->GetLeftVec());

    const float halfAngle = attackAngle * 0.5f;
    constexpr float yOffset = 0.06f;

    std::vector<glm::vec3> fanVertices;
    fanVertices.reserve(segments + 2);

    fanVertices.emplace_back(center + up * yOffset);

    for (int i = 0; i <= segments; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = glm::mix(-halfAngle, halfAngle, t);

        glm::vec3 dir = glm::normalize(forward * std::cos(angle) + left * std::sin(angle));

        fanVertices.emplace_back(center + dir * attackRange + up * yOffset);
    }

    DrawAttackRangeVertices(fanVertices, GL_TRIANGLE_FAN, glm::vec4(1.0f, 0.1f, 0.1f, 0.18f));

    constexpr float thickness = 0.08f;
    const float innerRadius = attackRange - thickness;
    const float outerRadius = attackRange;

    std::vector<glm::vec3> edgeVertices;
    edgeVertices.reserve((segments + 1) * 2);

    for (int i = 0; i <= segments; i++) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = glm::mix(-halfAngle, halfAngle, t);

        glm::vec3 dir = glm::normalize(forward * std::cos(angle) + left * std::sin(angle));

        edgeVertices.emplace_back(center + dir * outerRadius + up * yOffset);
        edgeVertices.emplace_back(center + dir * innerRadius + up * yOffset);
    }

    DrawAttackRangeVertices(edgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));
}

void Renderer3D::DrawEnemyAttackRange(Enemy* enemy) const
{
    if (!enemy) {
        return;
    }

    const float enemyAttackRange = enemy->GetAttackRange();

    if (enemyAttackRange <= 0.0f) {
        return;
    }

    const glm::vec3 center = enemy->GetPos();
    const glm::vec3 up = enemy->GetUpVec();
    const glm::vec3 forward = glm::normalize(enemy->GetFacingForwardVec());
    const glm::vec3 left = glm::normalize(enemy->GetLeftVec());
    const float enemyRadius = enemy->GetRadius() * enemy->GetScale().x;
    const glm::vec3 start = center + forward * enemy->GetRadius();
    const glm::vec3 end = start + forward * enemyAttackRange;

    std::vector<glm::vec3> fanVertices;
    fanVertices.reserve(4);

    constexpr float yOffset = 0.56f;

    fanVertices.emplace_back(start + left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(start + -left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    fanVertices.emplace_back(end + -left * enemyRadius + up * yOffset);

    DrawAttackRangeVertices(fanVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.18f));

    std::vector<glm::vec3> leftEdgeVertices;
    constexpr float thickness = 0.08f;

    leftEdgeVertices.emplace_back(start + left * enemyRadius + up * yOffset);
    leftEdgeVertices.emplace_back(start + left * (enemyRadius - thickness) + up * yOffset);
    leftEdgeVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    leftEdgeVertices.emplace_back(end + left * (enemyRadius - thickness) + up * yOffset);

    DrawAttackRangeVertices(leftEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    std::vector<glm::vec3> rightEdgeVertices;

    rightEdgeVertices.emplace_back(start - left * enemyRadius + up * yOffset);
    rightEdgeVertices.emplace_back(start - left * (enemyRadius - thickness) + up * yOffset);
    rightEdgeVertices.emplace_back(end - left * enemyRadius + up * yOffset);
    rightEdgeVertices.emplace_back(end - left * (enemyRadius - thickness) + up * yOffset);

    DrawAttackRangeVertices(rightEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));

    std::vector<glm::vec3> frontEdgeVertices;

    frontEdgeVertices.emplace_back(end + left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - forward * thickness + left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - left * enemyRadius + up * yOffset);
    frontEdgeVertices.emplace_back(end - forward * thickness - left * enemyRadius + up * yOffset);

    DrawAttackRangeVertices(frontEdgeVertices, GL_TRIANGLE_STRIP, glm::vec4(1.0f, 0.1f, 0.1f, 0.75f));
}

void Renderer3D::DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode,
                                         const glm::vec4& color) const
{
    if (vertices.empty()) {
        return;
    }

    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    glUniform4f(mShader3D->GetLocObjectColor(), color.r, color.g, color.b, color.a);

    StartTransparentDraw();

    glBindVertexArray(mAttackRangeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mAttackRangeVBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(drawMode, 0, static_cast<GLsizei>(vertices.size()));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    EndTransparentDraw();
}

void Renderer3D::DrawDebugLabels(const glm::mat4& viewMat) const
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const std::vector<Planet*>& planets = mGame->GetCurrentStage()->GetPlanets();

    for (auto planet : planets) {
        for (Platform* platform : planet->GetPlatforms()) {
            DrawDebugLabel(viewMat, platform, "足場 " + std::to_string(platform->GetStageYamlIndex()));
        }

        for (NPC* npc : planet->GetNPCs()) {
            DrawDebugLabel(viewMat, npc, "NPC " + std::to_string(npc->GetStageYamlIndex()));
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

        if (planet->GetKey()) {
            DrawDebugLabel(viewMat, planet->GetKey(), "鍵 " + std::to_string(planet->GetKey()->GetStageYamlIndex()));
        }

        if (planet->GetStar()) {
            DrawDebugLabel(viewMat, planet->GetStar(),
                           "スター " + std::to_string(planet->GetStar()->GetStageYamlIndex()));
        }
    }
}

void Renderer3D::DrawDebugLabel(const glm::mat4& viewMat, const Actor* actor, const std::string& label) const
{
    if (!actor || !actor->GetIsActive()) {
        return;
    }

    int textWidth = 0;
    int textHeight = 0;

    const SDL_Color textColor{255, 255, 255, 255};

    GLuint textTexture = CreateTextTexture(label, textWidth, textHeight, textColor, 1.0f);

    if (textTexture == 0 || textWidth <= 0 || textHeight <= 0) {
        return;
    }

    StartTransparentDraw();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glUniform1i(mShader3D->GetLocDiffuseTexture(), 0);
    glUniform1i(mShader3D->GetLocUseTexture(), 1);
    glUniform4f(mShader3D->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    mVertexArrays.at("quad")->SetActive();

    const float labelHeight = actor->GetRadius() * actor->GetScale().y + 0.8f;

    const float baseHeight = 0.5f;
    const float aspect = static_cast<float>(textWidth) / static_cast<float>(textHeight);

    const float height = baseHeight;
    const float width = baseHeight * aspect;

    glm::mat4 billboard = mGame->GetMathUtils()->CreateBillBoard(viewMat, actor, labelHeight, 0.0f, width, height);

    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(billboard));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUniform1i(mShader3D->GetLocUseTexture(), 0);

    EndTransparentDraw();

    glDeleteTextures(1, &textTexture);
}

glm::mat4 Renderer3D::CreateActorModelMatrix(Actor* actor, bool useOrient, float scaleMultiplier) const
{
    const glm::vec3 scale = actor->GetScale() * scaleMultiplier;

    if (useOrient) {
        return glm::translate(glm::mat4(1.0f), actor->GetPos()) * mGame->GetMathUtils()->CreateOrient(actor) *
               glm::scale(glm::mat4(1.0f), scale);
    }

    return glm::translate(glm::mat4(1.0f), actor->GetPos()) * glm::scale(glm::mat4(1.0f), scale);
}

void Renderer3D::DrawActorSelectionOutline(Actor* actor, bool useOrient) const
{
    if (!actor || !actor->GetIsActive()) {
        return;
    }

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        return;
    }

    constexpr float outlineScale = 1.06f;

    glm::mat4 model = CreateActorModelMatrix(actor, useOrient, outlineScale);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    GLint locObjectColor = mShader3D->GetLocObjectColor();
    GLint locUseTexture = mShader3D->GetLocUseTexture();

    glUniform1i(locUseTexture, 0);

    // オレンジ
    glUniform4f(locObjectColor, 1.0f, 0.45f, 0.0f, 1.0f);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    for (const auto& actorMesh : *actorMeshes) {
        glBindVertexArray(actorMesh.VAO);
        glDrawElements(GL_TRIANGLES, actorMesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}