#include "gfx/Renderer3D.h"

#include "Game.h"
#include "VertexArray.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "animation/SkeletalAnimationConstants.h"
#include "gfx/Shader3D.h"
#include "gfx/render3d/DebugLabelRenderer.h"
#include "gfx/render3d/NPCProximityMessageRenderer.h"
#include "gfx/render3d/PlayerEffectRenderer.h"
#include "gfx/particle/ParticleRenderer.h"
#include "gfx/render3d/RenderViewportController.h"
#include "gfx/render3d/SceneObjectRenderer.h"
#include "system/MeshLoadSystem.h"
#include "system/SceneSystem.h"
#include "system/mesh/LoadedMesh.h"
#include "system/mesh/TextureLoader.h"
#include "utils/MathUtils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>

namespace {
constexpr float EditorSelectionRed = 1.0f;
constexpr float EditorSelectionGreen = 0.45f;
constexpr float EditorSelectionBlue = 0.0f;
constexpr float EditorSelectionAlpha = 1.0f;

using SDLSurfacePtr =
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

struct RenderedRubySegment {
    SDLSurfacePtr baseSurface{nullptr, SDL_FreeSurface};
    SDLSurfacePtr rubySurface{nullptr, SDL_FreeSurface};
    int cellWidth = 0;
    int scaledRubyWidth = 0;
    int scaledRubyHeight = 0;
};

SDLSurfacePtr RenderTextSurface(
    TTF_Font* font,
    const std::string& text,
    const SDL_Color& textColor)
{
    if (!font || text.empty()) {
        return {nullptr, SDL_FreeSurface};
    }
    return {
        TTF_RenderUTF8_Blended(
            font,
            text.c_str(),
            textColor),
        SDL_FreeSurface};
}
} // namespace

Renderer3D::Renderer3D(Game* game)
    : Renderer(game),
      mShader3D(nullptr),
      mAttackRangeVAO(0),
      mAttackRangeVBO(0)
{
    Initialize();
}

Renderer3D::~Renderer3D() = default;

GLuint Renderer3D::CreateRubyTextTextureFor3D(
    const std::vector<RubyTextSegment>& segments,
    int& outWidth,
    int& outHeight,
    int& outBaseTextHeight,
    const SDL_Color textColor,
    float rubyScaleRatio,
    float rubyGapRatio) const
{
    outWidth = 0;
    outHeight = 0;
    outBaseTextHeight = 0;

    if (!mFont || segments.empty()) {
        return 0;
    }

    const float safeRubyScaleRatio =
        std::max(rubyScaleRatio, 0.1f);
    const float rubyOffsetMultiplier =
        std::max(0.0f, 0.9f + rubyGapRatio);

    std::vector<RenderedRubySegment> renderedSegments;
    renderedSegments.reserve(segments.size());

    int totalWidth = 0;
    int maximumRubyHeight = 0;
    int maximumRubyOffset = 0;

    for (const RubyTextSegment& segment : segments) {
        RenderedRubySegment renderedSegment;
        renderedSegment.baseSurface =
            RenderTextSurface(
                mFont,
                segment.text,
                textColor);
        if (!renderedSegment.baseSurface) {
            continue;
        }

        outBaseTextHeight =
            std::max(
                outBaseTextHeight,
                renderedSegment.baseSurface->h);

        if (segment.showsRuby &&
            !segment.reading.empty()) {
            renderedSegment.rubySurface =
                RenderTextSurface(
                    mFont,
                    segment.reading,
                    textColor);
        }

        if (renderedSegment.rubySurface) {
            renderedSegment.scaledRubyWidth =
                std::max(
                    1,
                    static_cast<int>(
                        std::lround(
                            renderedSegment.rubySurface->w *
                            safeRubyScaleRatio)));
            renderedSegment.scaledRubyHeight =
                std::max(
                    1,
                    static_cast<int>(
                        std::lround(
                            renderedSegment.rubySurface->h *
                            safeRubyScaleRatio)));
            maximumRubyHeight =
                std::max(
                    maximumRubyHeight,
                    renderedSegment.scaledRubyHeight);
            maximumRubyOffset =
                std::max(
                    maximumRubyOffset,
                    static_cast<int>(
                        std::ceil(
                            renderedSegment.scaledRubyHeight *
                            rubyOffsetMultiplier)));
        }

        renderedSegment.cellWidth =
            std::max(
                renderedSegment.baseSurface->w,
                renderedSegment.scaledRubyWidth);
        totalWidth += renderedSegment.cellWidth;
        renderedSegments.emplace_back(
            std::move(renderedSegment));
    }

    if (renderedSegments.empty() ||
        totalWidth <= 0 ||
        outBaseTextHeight <= 0) {
        return 0;
    }

    const int baseTextTop = maximumRubyOffset;
    const int textureHeight =
        std::max(
            maximumRubyHeight,
            baseTextTop + outBaseTextHeight);
    SDLSurfacePtr combinedSurface(
        SDL_CreateRGBSurfaceWithFormat(
            0,
            totalWidth,
            textureHeight,
            32,
            SDL_PIXELFORMAT_RGBA32),
        SDL_FreeSurface);
    if (!combinedSurface) {
        return 0;
    }

    SDL_FillRect(
        combinedSurface.get(),
        nullptr,
        SDL_MapRGBA(
            combinedSurface->format,
            0,
            0,
            0,
            0));

    int currentX = 0;
    for (const RenderedRubySegment& segment :
         renderedSegments) {
        SDL_SetSurfaceBlendMode(
            segment.baseSurface.get(),
            SDL_BLENDMODE_BLEND);
        SDL_Rect baseDestination{
            currentX +
                (segment.cellWidth -
                 segment.baseSurface->w) /
                    2,
            baseTextTop,
            segment.baseSurface->w,
            segment.baseSurface->h};
        SDL_BlitSurface(
            segment.baseSurface.get(),
            nullptr,
            combinedSurface.get(),
            &baseDestination);

        if (segment.rubySurface) {
            SDL_SetSurfaceBlendMode(
                segment.rubySurface.get(),
                SDL_BLENDMODE_BLEND);
            const int rubyVerticalOffset =
                static_cast<int>(
                    std::lround(
                        segment.scaledRubyHeight *
                        rubyOffsetMultiplier));
            SDL_Rect rubyDestination{
                currentX +
                    (segment.cellWidth -
                     segment.scaledRubyWidth) /
                        2,
                baseTextTop - rubyVerticalOffset,
                segment.scaledRubyWidth,
                segment.scaledRubyHeight};
            SDL_BlitScaled(
                segment.rubySurface.get(),
                nullptr,
                combinedSurface.get(),
                &rubyDestination);
        }

        currentX += segment.cellWidth;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        combinedSurface->w,
        combinedSurface->h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        combinedSurface->pixels);
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR);
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR);

    outWidth = combinedSurface->w;
    outHeight = combinedSurface->h;
    return texture;
}

void Renderer3D::Initialize()
{
    mShader3DUnique = std::make_unique<Shader3D>();
    mShader3D = mShader3DUnique.get();

    if (!mShader3D->GetShaderProgram()) {
        glfwTerminate();
        return;
    }

    const std::string basePath = "../assets/textures/";
    RegisterTexture(basePath + "guard.png", "guard");
    RegisterTexture(basePath + "tired_star.png", "tired_star");
    RegisterTexture(basePath + "textBg.png", "npcMessageBg");

    InitializeAttackRangeBuffer();
    InitializeRenderModules();
}

void Renderer3D::Draw() const
{
    if (!mGame || !mShader3D || !mRenderViewportController) {
        return;
    }

    if (mGame->GetSceneSystem()->IsTitle()) {
        return;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(mGame->GetWindow(), &framebufferWidth, &framebufferHeight);

    glUseProgram(mShader3D->GetShaderProgram());
    mRenderViewportController->DrawGameScreen(static_cast<float>(framebufferWidth),
                                              static_cast<float>(framebufferHeight));
}

void Renderer3D::DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos) const
{
    if (!mSceneObjectRenderer || !mShader3D) {
        return;
    }

    glUseProgram(mShader3D->GetShaderProgram());
    SetUniforms(viewMat, projMat, cameraPos);
    mSceneObjectRenderer->DrawSceneObjects(viewMat);

    if (mParticleRenderer) {
        mParticleRenderer->Draw(viewMat, projMat);
    }
}

void Renderer3D::InitializeAttackRangeBuffer()
{
    glGenVertexArrays(1, &mAttackRangeVAO);
    glGenBuffers(1, &mAttackRangeVBO);

    glBindVertexArray(mAttackRangeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mAttackRangeVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer3D::InitializeRenderModules()
{
    mPlayerEffectRenderer = std::make_unique<PlayerEffectRenderer>(this);
    mDebugLabelRenderer = std::make_unique<DebugLabelRenderer>(this);
    mNPCProximityMessageRenderer =
        std::make_unique<NPCProximityMessageRenderer>(this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(mGame);
    mSceneObjectRenderer =
        std::make_unique<SceneObjectRenderer>(
            this,
            mPlayerEffectRenderer.get(),
            mDebugLabelRenderer.get(),
            mNPCProximityMessageRenderer.get());
    mRenderViewportController = std::make_unique<RenderViewportController>(mGame, this);
}

void Renderer3D::SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat,
                             const glm::vec3& cameraPos) const
{
    glUniformMatrix4fv(mShader3D->GetLocView(), 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(mShader3D->GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));

    glUniform3f(mShader3D->GetLocViewPos(), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(mShader3D->GetLocLightPos(), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(mShader3D->GetLocLightColor(), 0.5f, 0.5f, 0.5f);

    glUniform1f(mShader3D->GetLocAmbientStrength(), 0.8f);
    glUniform1f(mShader3D->GetLocRimStrength(), 0.20f);
    glUniform1f(mShader3D->GetLocRimPower(), 2.5f);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform1i(mShader3D->GetLocBackTexture(), 1);
    glUniform1f(mShader3D->GetLocTextureSideBlendWidth(), 0.05f);
    SetSkinningEnabled(false);
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

void Renderer3D::TryDrawActor(Actor* actor, bool useOrient) const
{
    if (!actor || !actor->GetIsActive() ||
        actor->GetRenderOpacity() <= 0.001f) {
        return;
    }

    DrawActor(actor, useOrient);
}

void Renderer3D::DrawActor(Actor* actor, bool useOrient) const
{
    if (!actor || !mShader3D ||
        actor->GetRenderOpacity() <= 0.001f) {
        return;
    }

    SetSkinningEnabled(false);

    const bool isSelected = actor->GetIsEditorSelected();
    const bool isPlanet = dynamic_cast<const Planet*>(actor) != nullptr;
    if (isSelected && !isPlanet) {
        DrawActorSelectionUnderlay(actor, useOrient);
    }

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    const GLint objectColorLocation = mShader3D->GetLocObjectColor();
    const GLint useTextureLocation = mShader3D->GetLocUseTexture();
    const glm::vec2 textureTiling = actor->GetTextureTiling();
    glUniform2f(
        mShader3D->GetLocTextureTiling(),
        textureTiling.x,
        textureTiling.y);

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        return;
    }

    const float renderOpacity =
        glm::clamp(actor->GetRenderOpacity(), 0.0f, 1.0f);
    const bool transparent = renderOpacity < 0.999f;
    if (transparent) {
        StartTransparentDraw();
    }

    const bool hasUploadedSkinningMatrices = UploadActorSkinningMatrices(actor);
    const bool shouldRenderSolidWhite = actor->ShouldRenderSolidWhite();
    GLuint textureOverride = 0;
    if (!actor->GetTextureOverridePath().empty()) {
        textureOverride =
            const_cast<Renderer3D*>(this)->GetOrLoadTextureOverride(actor->GetTextureOverridePath());
    }

    GLuint backTextureOverride = 0;
    const Planet* planet = dynamic_cast<const Planet*>(actor);
    if (planet &&
        planet->GetPlanetShape() == Planet::PlanetShape::Ellipse &&
        !planet->GetBackTextureOverridePath().empty()) {
        backTextureOverride =
            const_cast<Renderer3D*>(this)->GetOrLoadTextureOverride(
                planet->GetBackTextureOverridePath());
    }

    if (backTextureOverride != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, backTextureOverride);
        glUniform1i(mShader3D->GetLocBackTexture(), 1);
        glUniform1i(mShader3D->GetLocUseBackTexture(), 1);
        glUniform1f(
            mShader3D->GetLocTextureSideBlendWidth(),
            planet->GetTextureSideBlendWidth());
        glActiveTexture(GL_TEXTURE0);
    } else {
        glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    }

    for (const LoadedMesh& actorMesh : *actorMeshes) {
        SetSkinningEnabled(hasUploadedSkinningMatrices && actorMesh.hasBoneInfluences);
        glBindVertexArray(actorMesh.VAO);

        const GLuint textureID = textureOverride != 0 ? textureOverride : actorMesh.textureID;
        if (textureID != 0 && !shouldRenderSolidWhite) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(mShader3D->GetLocDiffuseTexture(), 0);
            glUniform1i(useTextureLocation, 1);
        } else {
            glUniform1i(useTextureLocation, 0);
        }

        if (shouldRenderSolidWhite) {
            glUniform4f(objectColorLocation, 1.0f, 1.0f, 1.0f, renderOpacity);
        } else {
            glUniform4f(objectColorLocation, actorMesh.diffuseColor[0], actorMesh.diffuseColor[1],
                        actorMesh.diffuseColor[2], renderOpacity);
        }
        glDrawElements(GL_TRIANGLES, actorMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    if (transparent) {
        EndTransparentDraw();
    }

    SetSkinningEnabled(false);
    glUniform1i(useTextureLocation, 0);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform2f(mShader3D->GetLocTextureTiling(), 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);

    if (isSelected && isPlanet) {
        DrawActorSelectionOverlay(actor, useOrient);
    }
}

GLuint Renderer3D::GetOrLoadTextureOverride(const std::string& assetRelativePath)
{
    if (assetRelativePath.empty()) {
        return 0;
    }

    std::filesystem::path relativePath(assetRelativePath);
    relativePath = relativePath.lexically_normal();
    if (relativePath.is_absolute()) {
        return 0;
    }

    for (const auto& component : relativePath) {
        if (component == "..") {
            return 0;
        }
    }

    const std::string normalizedPath = relativePath.generic_string();
    const std::string textureKey = "actor_override:" + normalizedPath;
    const auto loaded = mTextures.find(textureKey);
    if (loaded != mTextures.end()) {
        return loaded->second;
    }
    if (mFailedTextureOverrides.contains(normalizedPath)) {
        return 0;
    }

    const std::filesystem::path fullPath =
        std::filesystem::path("../assets") / relativePath;
    TextureLoader textureLoader;
    const GLuint textureID = textureLoader.LoadTexture(fullPath.string().c_str());
    if (textureID == 0) {
        mFailedTextureOverrides.insert(normalizedPath);
        return 0;
    }

    mTextures[textureKey] = textureID;
    return textureID;
}

void Renderer3D::DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode,
                                         const glm::vec4& color) const
{
    if (!mShader3D || vertices.empty()) {
        return;
    }

    SetSkinningEnabled(false);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    glUniform4f(mShader3D->GetLocObjectColor(), color.r, color.g, color.b, color.a);

    StartTransparentDraw();

    glBindVertexArray(mAttackRangeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mAttackRangeVBO);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)), vertices.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(drawMode, 0, static_cast<GLsizei>(vertices.size()));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    EndTransparentDraw();
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

void Renderer3D::DrawActorSelectionUnderlay(
    Actor* actor,
    bool useOrient) const
{
    if (!actor || !actor->GetIsActive() || !mShader3D) {
        return;
    }

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        return;
    }

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(
        mShader3D->GetLocModel(),
        1,
        GL_FALSE,
        glm::value_ptr(model));

    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform4f(
        mShader3D->GetLocObjectColor(),
        EditorSelectionRed,
        EditorSelectionGreen,
        EditorSelectionBlue,
        EditorSelectionAlpha);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    const bool hasUploadedSkinningMatrices =
        UploadActorSkinningMatrices(actor);
    for (const LoadedMesh& actorMesh : *actorMeshes) {
        SetSkinningEnabled(
            hasUploadedSkinningMatrices &&
            actorMesh.hasBoneInfluences);
        glBindVertexArray(actorMesh.VAO);
        glDrawElements(
            GL_TRIANGLES,
            actorMesh.indexCount,
            GL_UNSIGNED_INT,
            nullptr);
    }

    SetSkinningEnabled(false);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
}

void Renderer3D::DrawActorSelectionOverlay(Actor* actor, bool useOrient) const
{
    if (!actor || !actor->GetIsActive() || !mShader3D) {
        return;
    }

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        return;
    }

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    const GLint objectColorLocation = mShader3D->GetLocObjectColor();
    const GLint useTextureLocation = mShader3D->GetLocUseTexture();

    glUniform1i(useTextureLocation, 0);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform4f(
        objectColorLocation,
        EditorSelectionRed,
        EditorSelectionGreen,
        EditorSelectionBlue,
        EditorSelectionAlpha);

    StartTransparentDraw();
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const bool hasUploadedSkinningMatrices = UploadActorSkinningMatrices(actor);
    for (const LoadedMesh& actorMesh : *actorMeshes) {
        SetSkinningEnabled(hasUploadedSkinningMatrices && actorMesh.hasBoneInfluences);
        glBindVertexArray(actorMesh.VAO);
        glDrawElements(GL_TRIANGLES, actorMesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    SetSkinningEnabled(false);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    EndTransparentDraw();
}

bool Renderer3D::UploadActorSkinningMatrices(const Actor* actor) const
{
    if (!actor || !mShader3D || mShader3D->GetLocBoneTransforms() < 0) {
        return false;
    }

    const std::vector<glm::mat4>* skinningMatrices = actor->GetSkinningMatrices();
    if (!skinningMatrices || skinningMatrices->empty()) {
        return false;
    }

    const std::size_t uploadCount =
        std::min(skinningMatrices->size(), SkeletalAnimationConstants::MaxShaderBoneCount);
    glUniformMatrix4fv(mShader3D->GetLocBoneTransforms(), static_cast<GLsizei>(uploadCount), GL_FALSE,
                       glm::value_ptr(skinningMatrices->front()));
    return true;
}

void Renderer3D::SetSkinningEnabled(bool isEnabled) const
{
    if (!mShader3D || mShader3D->GetLocUseSkinning() < 0) {
        return;
    }

    glUniform1i(mShader3D->GetLocUseSkinning(), isEnabled ? 1 : 0);
}
