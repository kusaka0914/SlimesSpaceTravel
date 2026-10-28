#include "gfx/Renderer3D.h"

#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/CharacterActor.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/Key.h"
#include "actor/MathUtils.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "animation/SkeletalAnimationConstants.h"
#include "component/PlatformMovementComponent.h"
#include "gfx/DirectionalShadowMap.h"
#include "gfx/DirectionalShadowShader.h"
#include "gfx/EnvironmentDecorationRenderer.h"
#include "gfx/PlanetAtmosphereRenderer.h"
#include "gfx/Shader3D.h"
#include "gfx/VertexArray.h"
#include "gfx/particle/ParticleRenderer.h"
#include "gfx/render3d/CharacterShadowSurface.h"
#include "gfx/render3d/DebugLabelRenderer.h"
#include "gfx/render3d/NPCProximityMessageRenderer.h"
#include "gfx/render3d/PlayerEffectRenderer.h"
#include "gfx/render3d/RenderViewportController.h"
#include "gfx/render3d/SceneObjectRenderer.h"
#include "system/PhysicsSystem.h"
#include "system/mesh/LoadedMesh.h"
#include "system/mesh/LoadedModel.h"
#include "system/mesh/TextureLoader.h"
#include "world/Stage.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <utility>

namespace {
constexpr float editorSelectionRed = 1.0f;
constexpr float editorSelectionGreen = 0.45f;
constexpr float editorSelectionBlue = 0.0f;
constexpr float editorSelectionAlpha = 1.0f;

// 指定色はsRGB。HDR合成後のガンマ補正で白っぽくならないよう、描画前にリニア化する。
const glm::vec4 mainPlayerSplitOutlineColor(glm::pow(glm::vec3(40.0f, 115.0f, 214.0f) / 255.0f, glm::vec3(2.2f)), 1.0f);
const glm::vec4 splitPlayerOutlineColor(glm::pow(glm::vec3(230.0f, 80.0f, 96.0f) / 255.0f, glm::vec3(2.2f)), 1.0f);
constexpr float splitPlayerOutlineScaleMultiplier = 1.10f;
constexpr int verticallyInvertedSunStageNumber = 0;

struct EmissiveAppearance {
    glm::vec3 color{1.0f};
    float intensity = 0.0f;
};

struct SurfaceLightingAppearance {
    float minimumReflectance = 0.0f;
    float rimBoost = 0.0f;
    float darkSurfaceLightBoost = 0.0f;
};

SurfaceLightingAppearance ResolveSurfaceLightingAppearance(const Actor* actor)
{
    if (dynamic_cast<const Platform*>(actor)) {
        return {0.055f, 0.105f, 0.20f};
    }
    if (dynamic_cast<const StageObject*>(actor)) {
        return {0.025f, 0.025f, 0.0f};
    }
    return {};
}

float ResolveColorSaturation(const Actor* actor)
{
    const Planet* planet = dynamic_cast<const Planet*>(actor);
    if (planet && planet->GetVisualSettings().biome == Planet::Biome::Grassland) {
        return 0.85f;
    }
    return 1.0f;
}

EmissiveAppearance ResolveEmissiveAppearance(const Actor* actor)
{
    if (dynamic_cast<const Star*>(actor)) {
        return {glm::vec3(1.0f, 0.82f, 0.28f), 1.35f};
    }
    if (dynamic_cast<const JewelItem*>(actor)) {
        return {glm::vec3(0.45f, 0.75f, 1.0f), 0.85f};
    }
    if (dynamic_cast<const Crystal*>(actor)) {
        return {glm::vec3(0.38f, 0.68f, 1.0f), 0.55f};
    }
    if (dynamic_cast<const Player*>(actor)) {
        return {glm::vec3(0.65f, 0.9f, 1.0f), 0.22f};
    }
    return {};
}

bool UsesTransparentShadowMaterial(const Actor* actor)
{
    if (!actor) {
        return false;
    }

    std::string texturePath = actor->GetRenderTextureOverridePath();
    std::transform(texturePath.begin(), texturePath.end(), texturePath.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return texturePath.find("glass") != std::string::npos || texturePath.find("transparent") != std::string::npos;
}

using SDLSurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

struct RenderedRubySegment {
    SDLSurfacePtr baseSurface{nullptr, SDL_FreeSurface};
    SDLSurfacePtr rubySurface{nullptr, SDL_FreeSurface};
    int cellWidth = 0;
    int scaledRubyWidth = 0;
    int scaledRubyHeight = 0;
};

std::vector<glm::vec3> CreateBlobShadowVertices(const glm::vec3& center, const glm::vec3& normal,
                                                const glm::vec3& tangentX, float radiusX, float radiusY)
{
    constexpr int segmentCount = 32;
    std::vector<glm::vec3> vertices;
    vertices.reserve(segmentCount + 2);
    vertices.push_back(center);

    const glm::vec3 tangentY = glm::normalize(glm::cross(normal, tangentX));
    for (int segment = 0; segment <= segmentCount; ++segment) {
        const float angle = glm::two_pi<float>() * static_cast<float>(segment) / static_cast<float>(segmentCount);
        vertices.push_back(center + tangentX * std::cos(angle) * radiusX + tangentY * std::sin(angle) * radiusY);
    }
    return vertices;
}

SDLSurfacePtr RenderTextSurface(TTF_Font* font, const std::string& text, const SDL_Color& textColor)
{
    if (!font || text.empty()) {
        return {nullptr, SDL_FreeSurface};
    }
    return {TTF_RenderUTF8_Blended(font, text.c_str(), textColor), SDL_FreeSurface};
}

std::vector<glm::vec3> CreateUGCPlacementGhostFaces(const glm::vec3& center, float gridSize)
{
    const float halfWidth = gridSize * 0.5f;
    const float halfHeight = gridSize * 0.1f;
    const glm::vec3 corners[] = {
        {center.x - halfWidth, center.y - halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y - halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y - halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y - halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y + halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y + halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y + halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y + halfHeight, center.z + halfWidth},
    };
    constexpr int indices[] = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7,
    };
    std::vector<glm::vec3> vertices;
    vertices.reserve(36);
    for (const int index : indices) {
        vertices.push_back(corners[index]);
    }
    return vertices;
}

std::vector<glm::vec3> CreateUGCPlacementGhostEdges(const glm::vec3& center, float gridSize)
{
    const float halfWidth = gridSize * 0.5f;
    const float halfHeight = gridSize * 0.1f;
    const glm::vec3 corners[] = {
        {center.x - halfWidth, center.y - halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y - halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y - halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y - halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y + halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y + halfHeight, center.z - halfWidth},
        {center.x + halfWidth, center.y + halfHeight, center.z + halfWidth},
        {center.x - halfWidth, center.y + halfHeight, center.z + halfWidth},
    };
    constexpr int edges[][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    std::vector<glm::vec3> vertices;
    vertices.reserve(24);
    for (const auto& edge : edges) {
        vertices.push_back(corners[edge[0]]);
        vertices.push_back(corners[edge[1]]);
    }
    return vertices;
}
} // namespace

Renderer3D::Renderer3D(Renderer3DDependencies dependencies)
    : mWindow(dependencies.window),
      mMath(dependencies.math),
      mModelLoader(dependencies.models),
      mPhysics(dependencies.physics),
      mUIDefinitions(dependencies.uiDefinitions),
      mCameras(dependencies.cameras),
      mPerformance(dependencies.performance),
      mReadScene(std::move(dependencies.readScene)),
      mShader3D(nullptr),
      mAttackRangeVAO(0),
      mAttackRangeVBO(0)
{
    Initialize();
}

Renderer3D::~Renderer3D()
{
    Shutdown();
}

void Renderer3D::Shutdown()
{
    if (mDirectionalShadowMap) {
        mDirectionalShadowMap->Shutdown();
    }
    if (mPlanetAtmosphereRenderer) {
        mPlanetAtmosphereRenderer->Shutdown();
    }
    if (mEnvironmentDecorationRenderer) {
        mEnvironmentDecorationRenderer->Shutdown();
    }
    if (mRenderViewportController) {
        mRenderViewportController->Shutdown();
    }
    if (mAttackRangeVAO != 0) {
        glDeleteVertexArrays(1, &mAttackRangeVAO);
        mAttackRangeVAO = 0;
    }
    if (mAttackRangeVBO != 0) {
        glDeleteBuffers(1, &mAttackRangeVBO);
        mAttackRangeVBO = 0;
    }
}

GLuint Renderer3D::CreateRubyTextTextureFor3D(const std::vector<RubyTextSegment>& segments, int& outWidth,
                                              int& outHeight, int& outBaseTextHeight, const SDL_Color textColor,
                                              float rubyScaleRatio, float rubyGapRatio) const
{
    outWidth = 0;
    outHeight = 0;
    outBaseTextHeight = 0;

    if (!mFont || segments.empty()) {
        return 0;
    }

    const float safeRubyScaleRatio = std::max(rubyScaleRatio, 0.1f);
    const float rubyOffsetMultiplier = std::max(0.0f, 0.9f + rubyGapRatio);

    std::vector<RenderedRubySegment> renderedSegments;
    renderedSegments.reserve(segments.size());

    int totalWidth = 0;
    int maximumRubyHeight = 0;
    int maximumRubyOffset = 0;

    for (const RubyTextSegment& segment : segments) {
        RenderedRubySegment renderedSegment;
        renderedSegment.baseSurface = RenderTextSurface(mFont, segment.text, textColor);
        if (!renderedSegment.baseSurface) {
            continue;
        }

        outBaseTextHeight = std::max(outBaseTextHeight, renderedSegment.baseSurface->h);

        if (segment.showsRuby && !segment.reading.empty()) {
            renderedSegment.rubySurface = RenderTextSurface(mFont, segment.reading, textColor);
        }

        if (renderedSegment.rubySurface) {
            renderedSegment.scaledRubyWidth =
                std::max(1, static_cast<int>(std::lround(renderedSegment.rubySurface->w * safeRubyScaleRatio)));
            renderedSegment.scaledRubyHeight =
                std::max(1, static_cast<int>(std::lround(renderedSegment.rubySurface->h * safeRubyScaleRatio)));
            maximumRubyHeight = std::max(maximumRubyHeight, renderedSegment.scaledRubyHeight);
            maximumRubyOffset =
                std::max(maximumRubyOffset,
                         static_cast<int>(std::ceil(renderedSegment.scaledRubyHeight * rubyOffsetMultiplier)));
        }

        // 読みが本文より長い場合も、隣の区画へ重ならない幅を確保する。
        renderedSegment.cellWidth = std::max(renderedSegment.baseSurface->w, renderedSegment.scaledRubyWidth);
        totalWidth += renderedSegment.cellWidth;
        renderedSegments.emplace_back(std::move(renderedSegment));
    }

    if (renderedSegments.empty() || totalWidth <= 0 || outBaseTextHeight <= 0) {
        return 0;
    }

    const int baseTextTop = maximumRubyOffset;
    const int textureHeight = std::max(maximumRubyHeight, baseTextTop + outBaseTextHeight);
    SDLSurfacePtr combinedSurface(
        SDL_CreateRGBSurfaceWithFormat(0, totalWidth, textureHeight, 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
    if (!combinedSurface) {
        return 0;
    }

    SDL_FillRect(combinedSurface.get(), nullptr, SDL_MapRGBA(combinedSurface->format, 0, 0, 0, 0));

    int currentX = 0;
    for (const RenderedRubySegment& segment : renderedSegments) {
        SDL_SetSurfaceBlendMode(segment.baseSurface.get(), SDL_BLENDMODE_BLEND);
        SDL_Rect baseDestination{currentX + (segment.cellWidth - segment.baseSurface->w) / 2, baseTextTop,
                                 segment.baseSurface->w, segment.baseSurface->h};
        SDL_BlitSurface(segment.baseSurface.get(), nullptr, combinedSurface.get(), &baseDestination);

        if (segment.rubySurface) {
            SDL_SetSurfaceBlendMode(segment.rubySurface.get(), SDL_BLENDMODE_BLEND);
            const int rubyVerticalOffset =
                static_cast<int>(std::lround(segment.scaledRubyHeight * rubyOffsetMultiplier));
            SDL_Rect rubyDestination{currentX + (segment.cellWidth - segment.scaledRubyWidth) / 2,
                                     baseTextTop - rubyVerticalOffset, segment.scaledRubyWidth,
                                     segment.scaledRubyHeight};
            SDL_BlitScaled(segment.rubySurface.get(), nullptr, combinedSurface.get(), &rubyDestination);
        }

        currentX += segment.cellWidth;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, combinedSurface->w, combinedSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 combinedSurface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    outWidth = combinedSurface->w;
    outHeight = combinedSurface->h;
    return texture;
}

void Renderer3D::Initialize()
{
    if (!mIsInitialized) {
        return;
    }

    mShader3DUnique = std::make_unique<Shader3D>();
    mShader3D = mShader3DUnique.get();

    if (!mShader3D->GetShaderProgram()) {
        // GameがGLリソースを解放してから終了できるよう、ここではGLFWを終了しない。
        mIsInitialized = false;
        return;
    }

    const std::string basePath = "../assets/textures/";
    RegisterTexture(basePath + "guard.png", "guard");
    RegisterTexture(basePath + "tired_star.png", "tired_star");
    RegisterTexture(basePath + "particles/spark_dot.png", "playerSoul");
    RegisterTexture(basePath + "textBg.png", "npcMessageBg");

    InitializeAttackRangeBuffer();
    InitializeRenderModules();
    mDirectionalShadowMap = std::make_unique<DirectionalShadowMap>();
    mPlanetAtmosphereRenderer = std::make_unique<PlanetAtmosphereRenderer>();
    mEnvironmentDecorationRenderer = std::make_unique<EnvironmentDecorationRenderer>();
}

void Renderer3D::Draw() const
{
    const SceneRenderState scene = mReadScene();
    if (!mShader3D || !mRenderViewportController) {
        return;
    }

    if (scene.isTitle || scene.isBattleStyleSelection) {
        return;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(&mWindow, &framebufferWidth, &framebufferHeight);

    glUseProgram(mShader3D->GetShaderProgram());
    mRenderViewportController->DrawGameScreen(static_cast<float>(framebufferWidth),
                                              static_cast<float>(framebufferHeight), scene);
}

void Renderer3D::RenderGameplayShadowMap() const
{
    const SceneRenderState scene = mReadScene();
    if (!scene.stage || scene.isTitle || scene.isBattleStyleSelection) {
        return;
    }

    const Player* controlledPlayer = scene.controlledPlayer;
    if (controlledPlayer) {
        RenderShadowMap(controlledPlayer->GetRenderPosition());
        return;
    }

    const std::vector<Planet*>& planets = scene.stage->GetPlanets();
    if (!planets.empty() && planets.front()) {
        RenderShadowMap(planets.front()->GetPos());
    }
}

void Renderer3D::RenderShadowMap(const glm::vec3& focusPosition) const
{
    const SceneRenderState scene = mReadScene();
    if (!scene.stage) {
        return;
    }
    const std::vector<Planet*>& planets = scene.stage->GetPlanets();
    const bool shouldDrawEnvironment = !scene.isUGCMode && mEnvironmentDecorationRenderer;
    if (shouldDrawEnvironment) {
        mEnvironmentDecorationRenderer->Prepare(planets, &mModelLoader);
    }

    if (!mDirectionalShadowMap ||
        !mDirectionalShadowMap->Begin(focusPosition, ResolveSunDirection(scene.stageNumber))) {
        return;
    }

    const auto drawActors = [this, &focusPosition](const auto& actors, bool useOrient) {
        for (auto* actor : actors) {
            DrawActorToShadowMap(actor, useOrient, focusPosition);
        }
    };

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }
        DrawActorToShadowMap(planet, false, focusPosition);
        drawActors(planet->GetEnemies(), true);
        for (Boat* boat : planet->GetBoats()) {
            if (!boat || boat->ShouldRenderUnavailablePreview()) {
                continue;
            }
            DrawActorToShadowMap(boat, true, focusPosition);
        }
        drawActors(planet->GetBoatParts(), true);
        drawActors(planet->GetCrystals(), true);
        drawActors(planet->GetPlatforms(), true);
        drawActors(planet->GetStageObjects(), true);
        drawActors(planet->GetNPCs(), true);
        drawActors(planet->GetJewelItems(), true);
        drawActors(planet->GetHazardActors(), true);
        DrawActorToShadowMap(planet->GetKey(), true, focusPosition);
        DrawActorToShadowMap(planet->GetStar(), true, focusPosition);
    }
    drawActors(scene.runtimeJewels, true);

    if (shouldDrawEnvironment) {
        DirectionalShadowShader* shadowShader = mDirectionalShadowMap->GetShader();
        if (shadowShader) {
            mEnvironmentDecorationRenderer->DrawShadow(*shadowShader, focusPosition);
        }
    }

    mDirectionalShadowMap->End();
}

void Renderer3D::DrawEnvironmentDecorations(const SceneRenderState& scene) const
{
    if (scene.isUGCMode || !scene.stage || !mEnvironmentDecorationRenderer || !mShader3D) {
        return;
    }
    mEnvironmentDecorationRenderer->Prepare(scene.stage->GetPlanets(), &mModelLoader);
    mEnvironmentDecorationRenderer->Draw(*mShader3D, mCurrentCameraPosition);
}

void Renderer3D::DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos,
                           UGCSceneLayerRenderMode ugcLayerRenderMode, int ugcEditLayer,
                           const Player* viewportPlayer) const
{
    const SceneRenderState scene = mReadScene();
    if (!mSceneObjectRenderer || !mShader3D) {
        return;
    }
    bool emphasizesUGCLayers = ugcLayerRenderMode == UGCSceneLayerRenderMode::HighlightEditingLayer ||
                               ugcLayerRenderMode == UGCSceneLayerRenderMode::HighlightEditingLayerWithoutDimming;
    bool dimsNonEditingUGCLayers = ugcLayerRenderMode == UGCSceneLayerRenderMode::HighlightEditingLayer;
    const bool shouldAutomaticallyHighlightEditingLayer =
        ugcLayerRenderMode == UGCSceneLayerRenderMode::AutomaticallyHighlightEditingLayer && scene.isUGCMode &&
        scene.isDebugEditorShowing;
    if (shouldAutomaticallyHighlightEditingLayer) {
        emphasizesUGCLayers = true;
        dimsNonEditingUGCLayers = true;
        ugcEditLayer = scene.ugcEditLayer;
    }

    glUseProgram(mShader3D->GetShaderProgram());
    mCurrentCameraPosition = cameraPos;
    mEmphasizeUGCLayers = emphasizesUGCLayers;
    mDimNonEditingUGCLayers = dimsNonEditingUGCLayers;
    mUGCEditLayer = ugcEditLayer;
    UpdateViewFrustum(projMat * viewMat);
    SetUniforms(viewMat, projMat, cameraPos, scene.stageNumber);
    mSceneObjectRenderer->DrawSceneObjects(viewMat, viewportPlayer, scene);

    if (mPlanetAtmosphereRenderer && scene.stage) {
        mPlanetAtmosphereRenderer->Draw(scene.stage->GetPlanets(), viewMat, projMat, cameraPos,
                                        ResolveSunDirection(scene.stageNumber));
        glUseProgram(mShader3D->GetShaderProgram());
        SetUniforms(viewMat, projMat, cameraPos, scene.stageNumber);
    }

    if (scene.isUGCMode && scene.isDebugEditorShowing) {
        DrawUGCMovingPlatformPaths(scene);
        const std::optional<glm::vec3>& placementPreview = scene.placementPosition;
        if (placementPreview) {
            Actor* placementPreviewActor = scene.placementModel;
            if (placementPreviewActor) {
                const auto& previewPositions = scene.placementModelPositions;
                if (previewPositions.empty()) {
                    DrawUGCPlacementPreviewActor(placementPreviewActor);
                } else {
                    for (const glm::vec3& previewPosition : previewPositions) {
                        placementPreviewActor->SetPos(previewPosition);
                        DrawUGCPlacementPreviewActor(placementPreviewActor);
                    }
                }
            } else {
                const float gridSize = scene.ugcGridSize;
                DrawAttackRangeVertices(CreateUGCPlacementGhostFaces(*placementPreview, gridSize), GL_TRIANGLES,
                                        glm::vec4(0.25f, 0.95f, 0.68f, 0.32f));
                DrawAttackRangeVertices(CreateUGCPlacementGhostEdges(*placementPreview, gridSize), GL_LINES,
                                        glm::vec4(0.62f, 1.0f, 0.82f, 0.95f));
            }

            SetUniforms(viewMat, projMat, cameraPos, scene.stageNumber);
        }
    }

    if (mParticleRenderer) {
        mParticleRenderer->Draw(viewMat, projMat, scene.particles);
    }
    mEmphasizeUGCLayers = false;
    mDimNonEditingUGCLayers = false;
}

void Renderer3D::DrawUGCMovingPlatformPaths(const SceneRenderState& scene) const
{
    if (!scene.stage) {
        return;
    }

    std::vector<glm::vec3> pathVertices;
    std::vector<std::pair<Platform*, glm::vec3>> endpointPreviews;
    const float pathHeightOffset = scene.ugcGridSize * 0.18f;
    for (Planet* planet : scene.stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform || !platform->GetIsActive()) {
                continue;
            }

            const PlatformMovementComponent* movement = platform->GetMovementComponent();
            if (!movement || !platform->GetIsUGCGenerated()) {
                continue;
            }

            const glm::vec3 sourceWorldPosition = planet->GetPos() + movement->GetBaseLocalPos();
            const glm::vec3 destinationWorldPosition = planet->GetPos() + movement->GetDestinationLocalPos();
            pathVertices.push_back(sourceWorldPosition + glm::vec3(0.0f, pathHeightOffset, 0.0f));
            pathVertices.push_back(destinationWorldPosition + glm::vec3(0.0f, pathHeightOffset, 0.0f));
            const glm::vec3 currentWorldPosition = platform->GetPos();
            const glm::vec3 otherEndpointWorldPosition =
                glm::distance(currentWorldPosition, sourceWorldPosition) <=
                        glm::distance(currentWorldPosition, destinationWorldPosition)
                    ? destinationWorldPosition
                    : sourceWorldPosition;
            endpointPreviews.emplace_back(platform, otherEndpointWorldPosition);
        }
    }

    const std::optional<glm::vec3>& previewPathStart = scene.movingPathStart;
    const std::optional<glm::vec3>& previewPathDestination = scene.movingPathDestination;
    if (previewPathStart && previewPathDestination) {
        pathVertices.push_back(*previewPathStart + glm::vec3(0.0f, pathHeightOffset, 0.0f));
        pathVertices.push_back(*previewPathDestination + glm::vec3(0.0f, pathHeightOffset, 0.0f));
    }

    if (pathVertices.empty()) {
        return;
    }

    GLboolean wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    DrawAttackRangeVertices(pathVertices, GL_LINES, glm::vec4(0.2f, 0.9f, 1.0f, 0.9f), 2.0f);
    if (wasDepthTestEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }

    for (const auto& [platform, endpointWorldPosition] : endpointPreviews) {
        const glm::vec3 currentWorldPosition = platform->GetPos();
        platform->SetPos(endpointWorldPosition);
        DrawUGCPlacementPreviewActor(platform);
        platform->SetPos(currentWorldPosition);
    }
}

void Renderer3D::UpdateViewFrustum(const glm::mat4& viewProjectionMatrix) const
{
    // GLMの行列は列から添字を取るため、クリップ空間の6平面を抽出する前に行を組み立てる。
    const glm::vec4 firstRow(viewProjectionMatrix[0][0], viewProjectionMatrix[1][0], viewProjectionMatrix[2][0],
                             viewProjectionMatrix[3][0]);
    const glm::vec4 secondRow(viewProjectionMatrix[0][1], viewProjectionMatrix[1][1], viewProjectionMatrix[2][1],
                              viewProjectionMatrix[3][1]);
    const glm::vec4 thirdRow(viewProjectionMatrix[0][2], viewProjectionMatrix[1][2], viewProjectionMatrix[2][2],
                             viewProjectionMatrix[3][2]);
    const glm::vec4 fourthRow(viewProjectionMatrix[0][3], viewProjectionMatrix[1][3], viewProjectionMatrix[2][3],
                              viewProjectionMatrix[3][3]);

    mViewFrustumPlanes = {
        fourthRow + firstRow,  fourthRow - firstRow, fourthRow + secondRow,
        fourthRow - secondRow, fourthRow + thirdRow, fourthRow - thirdRow,
    };

    mHasValidViewFrustum = true;
    for (glm::vec4& plane : mViewFrustumPlanes) {
        const float normalLength = glm::length(glm::vec3(plane));
        if (normalLength <= 0.000001f) {
            mHasValidViewFrustum = false;
            return;
        }
        plane /= normalLength;
    }
}

bool Renderer3D::IsActorInsideView(const Actor* actor) const
{
    if (!actor || !mHasValidViewFrustum) {
        return true;
    }

    const glm::vec3 absoluteScale = glm::abs(actor->GetScale());
    const float maximumScale = std::max(absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z));
    const float boundingRadius = std::max(0.5f, actor->GetRadius() * maximumScale) + 1.0f;

    for (const glm::vec4& plane : mViewFrustumPlanes) {
        const float signedDistance = glm::dot(glm::vec3(plane), actor->GetPos()) + plane.w;
        if (signedDistance < -boundingRadius) {
            return false;
        }
    }

    return true;
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
    mNPCProximityMessageRenderer = std::make_unique<NPCProximityMessageRenderer>(this, mUIDefinitions);
    mParticleRenderer = std::make_unique<ParticleRenderer>();
    mSceneObjectRenderer = std::make_unique<SceneObjectRenderer>(
        this, mPhysics, mPlayerEffectRenderer.get(), mDebugLabelRenderer.get(), mNPCProximityMessageRenderer.get());
    mRenderViewportController = std::make_unique<RenderViewportController>(mCameras, mPerformance, this);
}

glm::vec3 Renderer3D::ResolveSunDirection(int stageNumber) const
{
    glm::vec3 sunDirection = mLightingSettings.sunDirection;
    if (stageNumber == verticallyInvertedSunStageNumber) {
        sunDirection.y = -sunDirection.y;
    }
    return sunDirection;
}

void Renderer3D::SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos,
                             int stageNumber) const
{
    glUniformMatrix4fv(mShader3D->GetLocView(), 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(mShader3D->GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));

    glUniform3f(mShader3D->GetLocCameraWorldPosition(), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3fv(mShader3D->GetLocSunDirection(), 1, glm::value_ptr(ResolveSunDirection(stageNumber)));
    glUniform3fv(mShader3D->GetLocSunColor(), 1, glm::value_ptr(mLightingSettings.sunColor));
    glUniform1f(mShader3D->GetLocSunIntensity(), mLightingSettings.sunIntensity);
    glUniform3fv(mShader3D->GetLocEnvironmentColor(), 1, glm::value_ptr(mLightingSettings.environmentColor));
    glUniform1f(mShader3D->GetLocDayEnvironmentIntensity(), mLightingSettings.dayEnvironmentIntensity);
    glUniform1f(mShader3D->GetLocNightEnvironmentIntensity(), mLightingSettings.nightEnvironmentIntensity);
    glUniform3fv(mShader3D->GetLocRimColor(), 1, glm::value_ptr(mLightingSettings.rimColor));
    glUniform1f(mShader3D->GetLocDayRimStrength(), mLightingSettings.dayRimStrength);
    glUniform1f(mShader3D->GetLocNightRimStrength(), mLightingSettings.nightRimStrength);
    glUniform1f(mShader3D->GetLocRimPower(), mLightingSettings.rimPower);
    glUniform1f(mShader3D->GetLocMaterialMinimumReflectance(), 0.0f);
    glUniform1f(mShader3D->GetLocMaterialRimBoost(), 0.0f);
    glUniform1f(mShader3D->GetLocMaterialDarkSurfaceLightBoost(), 0.0f);
    glUniform1i(mShader3D->GetLocIsUnlit(), 0);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform1i(mShader3D->GetLocBackTexture(), 1);
    glUniform1f(mShader3D->GetLocTextureSideBlendWidth(), 0.05f);
    glUniform3f(mShader3D->GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);
    glUniform1f(mShader3D->GetLocColorSaturation(), 1.0f);
    glUniform1i(mShader3D->GetLocApplyOutputGamma(), 0);
    glUniform3f(mShader3D->GetLocEmissiveColor(), 1.0f, 1.0f, 1.0f);
    glUniform1f(mShader3D->GetLocEmissiveIntensity(), 0.0f);
    glUniform1f(mShader3D->GetLocHardeningAmount(), 0.0f);
    glUniform1i(mShader3D->GetLocUseInstancing(), 0);
    const bool shadowsAvailable = mDirectionalShadowMap && mDirectionalShadowMap->IsAvailable();
    glUniform1i(mShader3D->GetLocUseShadows(), shadowsAvailable ? 1 : 0);
    if (shadowsAvailable) {
        const DirectionalShadowSettings& shadowSettings = mDirectionalShadowMap->GetSettings();
        glUniformMatrix4fv(mShader3D->GetLocLightSpaceMatrix(), 1, GL_FALSE,
                           glm::value_ptr(mDirectionalShadowMap->GetLightSpaceMatrix()));
        glUniform1f(mShader3D->GetLocShadowMinimumBias(), shadowSettings.minimumBias);
        glUniform1f(mShader3D->GetLocShadowMaximumBias(), shadowSettings.maximumBias);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mDirectionalShadowMap->GetDepthTexture());
        glUniform1i(mShader3D->GetLocShadowMap(), 2);
        glActiveTexture(GL_TEXTURE0);
    }
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

void Renderer3D::TryDrawActor(Actor* actor, bool useOrient, const PlayerSoulPresentation& soul) const
{
    if (!actor || !actor->GetIsActive() || actor->GetRenderOpacity() <= 0.001f) {
        return;
    }

    DrawActor(actor, useOrient, soul);
}

void Renderer3D::DrawUGCPlacementPreviewActor(Actor* actor) const
{
    DrawInactiveActorPreview(actor, 0.42f);
}

void Renderer3D::DrawInactiveActorPreview(Actor* actor, float opacity) const
{
    if (!actor) {
        return;
    }

    mActorOpacityMultiplier = glm::clamp(opacity, 0.0f, 1.0f);
    DrawActor(actor);
    mActorOpacityMultiplier = 1.0f;
}

void Renderer3D::DrawBlobShadow(const CharacterActor* actor) const
{
    if (!actor || !actor->GetIsActive() || actor->GetRenderOpacity() <= 0.001f || !mShader3D) {
        return;
    }
    const auto surface = CharacterShadow::FindSurface(*actor, mPhysics);
    if (!surface) {
        return;
    }
    glm::vec3 shadowCenter = surface->position;
    const auto surfaceNormal = surface->normal;
    const float surfaceDistance = surface->distance;
    const auto* player = dynamic_cast<const Player*>(actor);
    const bool isPlayer = player != nullptr;

    glm::vec3 tangentX = actor->GetRightVec() - surfaceNormal * glm::dot(actor->GetRightVec(), surfaceNormal);
    if (glm::length(tangentX) <= 0.0001f) {
        tangentX = glm::cross(surfaceNormal, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    if (glm::length(tangentX) <= 0.0001f) {
        tangentX = glm::cross(surfaceNormal, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    tangentX = glm::normalize(tangentX);

    float radiusX = 0.34f;
    float radiusY = 0.27f;
    if (player) {
        // スキニング前の頂点境界は表示寸法と一致しないため、Playerはゲーム内の体サイズを使う。
        constexpr float playerShadowBodySizeRatio = 0.75f;
        const float halfCollisionScale = player->GetCollisionScaleMultiplier() * 0.5f * playerShadowBodySizeRatio;
        radiusX = std::max(mPhysics.GetPlayerCollisionWidth() * halfCollisionScale, 0.05f);
        radiusY = std::max(mPhysics.GetPlayerCollisionDepth() * halfCollisionScale, 0.05f);
    }
    const LoadedModel* model = actor->GetLoadedModel();
    if (!isPlayer && model && model->hasBounds) {
        const glm::vec3 modelSize = glm::abs((model->boundsMaximum - model->boundsMinimum) * actor->GetScale());
        radiusX = glm::clamp(modelSize.x * 0.31f, 0.21f, 2.5f);
        radiusY = glm::clamp(modelSize.z * 0.31f, 0.17f, 2.5f);
    }

    shadowCenter += surfaceNormal * 0.012f;
    const float distanceScale = isPlayer ? 1.0f : 1.0f + glm::clamp(surfaceDistance * 0.025f, 0.0f, 0.22f);
    radiusX *= distanceScale;
    radiusY *= distanceScale;

    GLint previousUnlit = GL_FALSE;
    glGetUniformiv(mShader3D->GetShaderProgram(), mShader3D->GetLocIsUnlit(), &previousUnlit);
    // 裏側や建物の影の中でも、着地の目印となる簡易影は照明に依存させない。
    glUniform1i(mShader3D->GetLocIsUnlit(), 1);
    if (isPlayer) {
        constexpr float playerShadowOpacity = 0.4f;
        DrawAttackRangeVertices(CreateBlobShadowVertices(shadowCenter, surfaceNormal, tangentX, radiusX, radiusY),
                                GL_TRIANGLE_FAN, glm::vec4(0.0f, 0.0f, 0.0f, playerShadowOpacity));
    } else {
        struct ShadowLayer {
            float scale;
            float alpha;
        };
        constexpr ShadowLayer layers[] = {
            {1.00f, 0.17f},
            {0.76f, 0.25f},
            {0.50f, 0.36f},
        };
        for (const ShadowLayer& layer : layers) {
            DrawAttackRangeVertices(CreateBlobShadowVertices(shadowCenter, surfaceNormal, tangentX,
                                                             radiusX * layer.scale, radiusY * layer.scale),
                                    GL_TRIANGLE_FAN, glm::vec4(0.0f, 0.0f, 0.0f, layer.alpha));
        }
    }
    glUniform1i(mShader3D->GetLocIsUnlit(), previousUnlit);
}

void Renderer3D::DrawActor(Actor* actor, bool useOrient, const PlayerSoulPresentation& soul) const
{
    if (!actor || !mShader3D || actor->GetRenderOpacity() <= 0.001f) {
        return;
    }

    SetSkinningEnabled(false);

    const auto* player = dynamic_cast<const Player*>(actor);
    // 分裂中の識別色は操作対象ではなく個体番号に固定する。
    if (player && player->IsSplitForm()) {
        const auto& outlineColor = player->GetPlayerNum() == 2 ? splitPlayerOutlineColor : mainPlayerSplitOutlineColor;
        GLboolean previousDepthWriteMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWriteMask);
        // 拡大メッシュが深度を書き込むと、面の向きによって後描きの本体まで隠してしまう。
        glDepthMask(GL_FALSE);
        glUniform1i(mShader3D->GetLocIsUnlit(), 1);
        DrawActorOutlineUnderlay(actor, useOrient, outlineColor, splitPlayerOutlineScaleMultiplier);
        glUniform1i(mShader3D->GetLocIsUnlit(), 0);
        glDepthMask(previousDepthWriteMask);
    }

    const bool isSelected = actor->GetIsEditorSelected();
    const bool isPlanet = dynamic_cast<const Planet*>(actor) != nullptr;
    const Platform* platform = dynamic_cast<const Platform*>(actor);
    const bool isGeneratedUGCPlatform = mEmphasizeUGCLayers && platform && platform->GetIsUGCGenerated();
    const bool isOnCurrentUGCLayer = isGeneratedUGCPlatform && platform->GetUGCGridLayer() == mUGCEditLayer;
    if (isSelected && !isPlanet) {
        DrawActorSelectionUnderlay(actor, useOrient);
    } else if (isOnCurrentUGCLayer) {
        DrawActorOutlineUnderlay(actor, useOrient, glm::vec4(1.0f, 0.78f, 0.16f, 1.0f), 1.045f);
    }

    float layerBrightness = 1.0f;
    if (mDimNonEditingUGCLayers && isGeneratedUGCPlatform && !isOnCurrentUGCLayer) {
        const int layerDistance = std::abs(platform->GetUGCGridLayer() - mUGCEditLayer);
        layerBrightness = layerDistance == 1 ? 0.45f : 0.22f;
    }
    const bool hasSoulAppearance = player && player->IsSplitForm() && soul.controlledPlayer;
    const bool containsSoul = hasSoulAppearance && player == soul.controlledPlayer;
    const bool isHardened = hasSoulAppearance && !containsSoul;
    const float bodyBrightness = isHardened ? 0.50f : 1.0f;
    const float brightness = layerBrightness * bodyBrightness;
    glUniform3f(mShader3D->GetLocColorMultiplier(), brightness, brightness, brightness);
    glUniform1f(mShader3D->GetLocColorSaturation(), isHardened ? 0.55f : ResolveColorSaturation(actor));

    EmissiveAppearance emissiveAppearance = ResolveEmissiveAppearance(actor);
    if (isHardened) {
        emissiveAppearance.intensity = 0.08f;
    }
    glUniform3fv(mShader3D->GetLocEmissiveColor(), 1, glm::value_ptr(emissiveAppearance.color));
    glUniform1f(mShader3D->GetLocEmissiveIntensity(), emissiveAppearance.intensity);

    SurfaceLightingAppearance surfaceLightingAppearance = ResolveSurfaceLightingAppearance(actor);
    if (isHardened) {
        surfaceLightingAppearance = {0.035f, 0.02f, 0.08f};
    }
    glUniform1f(mShader3D->GetLocMaterialMinimumReflectance(), surfaceLightingAppearance.minimumReflectance);
    glUniform1f(mShader3D->GetLocMaterialRimBoost(), surfaceLightingAppearance.rimBoost);
    glUniform1f(mShader3D->GetLocMaterialDarkSurfaceLightBoost(), surfaceLightingAppearance.darkSurfaceLightBoost);
    glUniform1f(mShader3D->GetLocHardeningAmount(), isHardened ? 1.0f : 0.0f);

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    const GLint objectColorLocation = mShader3D->GetLocObjectColor();
    const GLint useTextureLocation = mShader3D->GetLocUseTexture();
    const glm::vec2 textureTiling = actor->GetRenderTextureTiling();
    glUniform2f(mShader3D->GetLocTextureTiling(), textureTiling.x, textureTiling.y);

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        glUniform3f(mShader3D->GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);
        glUniform1f(mShader3D->GetLocColorSaturation(), 1.0f);
        glUniform1f(mShader3D->GetLocEmissiveIntensity(), 0.0f);
        glUniform1f(mShader3D->GetLocHardeningAmount(), 0.0f);
        glUniform1f(mShader3D->GetLocMaterialMinimumReflectance(), 0.0f);
        glUniform1f(mShader3D->GetLocMaterialRimBoost(), 0.0f);
        glUniform1f(mShader3D->GetLocMaterialDarkSurfaceLightBoost(), 0.0f);
        return;
    }

    const float renderOpacity = glm::clamp(actor->GetRenderOpacity() * mActorOpacityMultiplier, 0.0f, 1.0f);
    const bool transparent = renderOpacity < 0.999f;
    if (transparent) {
        StartTransparentDraw();
    }

    const bool hasUploadedSkinningMatrices = UploadActorSkinningMatrices(actor);
    const bool shouldRenderSolidWhite = actor->ShouldRenderSolidWhite();
    GLuint textureOverride = 0;
    const std::string& renderTextureOverridePath = actor->GetRenderTextureOverridePath();
    if (!renderTextureOverridePath.empty()) {
        textureOverride = GetOrLoadTextureOverride(renderTextureOverridePath);
    }

    GLuint backTextureOverride = 0;
    const Planet* planet = dynamic_cast<const Planet*>(actor);
    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Ellipse &&
        !planet->GetBackTextureOverridePath().empty()) {
        backTextureOverride = GetOrLoadTextureOverride(planet->GetBackTextureOverridePath());
    }

    if (backTextureOverride != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, backTextureOverride);
        glUniform1i(mShader3D->GetLocBackTexture(), 1);
        glUniform1i(mShader3D->GetLocUseBackTexture(), 1);
        glUniform1f(mShader3D->GetLocTextureSideBlendWidth(), planet->GetTextureSideBlendWidth());
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
    glUniform3f(mShader3D->GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);
    glUniform1f(mShader3D->GetLocColorSaturation(), 1.0f);
    glUniform3f(mShader3D->GetLocEmissiveColor(), 1.0f, 1.0f, 1.0f);
    glUniform1f(mShader3D->GetLocEmissiveIntensity(), 0.0f);
    glUniform1f(mShader3D->GetLocHardeningAmount(), 0.0f);
    glUniform1f(mShader3D->GetLocMaterialMinimumReflectance(), 0.0f);
    glUniform1f(mShader3D->GetLocMaterialRimBoost(), 0.0f);
    glUniform1f(mShader3D->GetLocMaterialDarkSurfaceLightBoost(), 0.0f);
}

VertexArray* Renderer3D::FindVertexArray(const std::string& name) const
{
    const auto vertexArray = mVertexArrays.find(name);
    return vertexArray != mVertexArrays.end() ? vertexArray->second.get() : nullptr;
}

GLuint Renderer3D::FindTexture(const std::string& name) const
{
    const auto texture = mTextures.find(name);
    return texture != mTextures.end() ? texture->second : 0;
}

GLuint Renderer3D::GetOrLoadTextureOverride(const std::string& assetRelativePath) const
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

    // 描画のたびに失敗したファイルを読み直さないよう、失敗結果もキャッシュする。
    if (mFailedTextureOverrides.contains(normalizedPath)) {
        return 0;
    }

    const std::filesystem::path fullPath = std::filesystem::path("../assets") / relativePath;
    TextureLoader textureLoader;
    const GLuint textureID = textureLoader.LoadTexture(fullPath.string().c_str(), TextureColorSpace::SRGB);
    if (textureID == 0) {
        mFailedTextureOverrides.insert(normalizedPath);
        return 0;
    }

    mTextures[textureKey] = textureID;
    return textureID;
}

void Renderer3D::DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode,
                                         const glm::vec4& color, std::optional<float> lineWidth) const
{
    if (!mShader3D || vertices.empty()) {
        return;
    }

    SetSkinningEnabled(false);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    glUniform4f(mShader3D->GetLocObjectColor(), color.r, color.g, color.b, color.a);

    const bool overridesLineWidth =
        lineWidth && (drawMode == GL_LINES || drawMode == GL_LINE_STRIP || drawMode == GL_LINE_LOOP);
    GLfloat previousLineWidth = 1.0f;
    if (overridesLineWidth) {
        GLfloat widthRange[2]{};
        glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
        glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, widthRange);
        // 幅の広い線を受け付けない実装でも、GLエラーを起こさず同じプリミティブを描く。
        glLineWidth(std::clamp(*lineWidth, widthRange[0], widthRange[1]));
    }
    StartTransparentDraw();

    glBindVertexArray(mAttackRangeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mAttackRangeVBO);

    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)), vertices.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(drawMode, 0, static_cast<GLsizei>(vertices.size()));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    EndTransparentDraw();
    if (overridesLineWidth) {
        glLineWidth(previousLineWidth);
    }
}

glm::mat4 Renderer3D::CreateBillboard(const glm::mat4& viewMat, const Actor* actor, float upMargin, float rightMargin,
                                      float width, float height) const
{
    return mMath.CreateBillBoard(viewMat, actor, upMargin, rightMargin, width, height);
}

glm::mat4 Renderer3D::CreateBillboard(const glm::mat4& viewMat, const glm::vec3& centerPosition,
                                      const glm::vec3& upDirection, float width, float height) const
{
    return mMath.CreateBillBoard(viewMat, centerPosition, upDirection, width, height);
}

glm::mat4 Renderer3D::CreateActorModelMatrix(Actor* actor, bool useOrient, float scaleMultiplier) const
{
    const glm::vec3 position = actor->GetRenderPosition();
    const glm::vec3 scale = actor->GetRenderScale() * scaleMultiplier;

    if (useOrient) {
        return glm::translate(glm::mat4(1.0f), position) * mMath.CreateOrient(actor) *
               glm::scale(glm::mat4(1.0f), scale);
    }

    return glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);
}

void Renderer3D::DrawActorSelectionUnderlay(Actor* actor, bool useOrient) const
{
    DrawActorOutlineUnderlay(
        actor, useOrient,
        glm::vec4(editorSelectionRed, editorSelectionGreen, editorSelectionBlue, editorSelectionAlpha), 1.0f);
}

void Renderer3D::DrawActorOutlineUnderlay(Actor* actor, bool useOrient, const glm::vec4& color,
                                          float scaleMultiplier) const
{
    if (!actor || !actor->GetIsActive() || !mShader3D) {
        return;
    }

    const std::vector<LoadedMesh>* actorMeshes = actor->GetMeshes();
    if (!actorMeshes || actorMeshes->empty()) {
        glUniform3f(mShader3D->GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);
        return;
    }

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, scaleMultiplier);
    glUniformMatrix4fv(mShader3D->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(mShader3D->GetLocUseTexture(), 0);
    glUniform1i(mShader3D->GetLocUseBackTexture(), 0);
    glUniform3f(mShader3D->GetLocColorMultiplier(), 1.0f, 1.0f, 1.0f);
    glUniform4f(mShader3D->GetLocObjectColor(), color.r, color.g, color.b, color.a);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    const bool hasUploadedSkinningMatrices = UploadActorSkinningMatrices(actor);
    for (const LoadedMesh& actorMesh : *actorMeshes) {
        SetSkinningEnabled(hasUploadedSkinningMatrices && actorMesh.hasBoneInfluences);
        glBindVertexArray(actorMesh.VAO);
        glDrawElements(GL_TRIANGLES, actorMesh.indexCount, GL_UNSIGNED_INT, nullptr);
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
    glUniform4f(objectColorLocation, editorSelectionRed, editorSelectionGreen, editorSelectionBlue,
                editorSelectionAlpha);

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

    const std::size_t uploadCount = std::min(skinningMatrices->size(), SkeletalAnimationConstants::MaxShaderBoneCount);
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

void Renderer3D::DrawActorToShadowMap(Actor* actor, bool useOrient, const glm::vec3& focusPosition) const
{
    if (!actor || !actor->GetIsActive() || actor->GetRenderOpacity() < 0.999f || UsesTransparentShadowMaterial(actor) ||
        !mDirectionalShadowMap) {
        return;
    }

    const DirectionalShadowSettings& shadowSettings = mDirectionalShadowMap->GetSettings();
    const glm::vec3 absoluteScale = glm::abs(actor->GetRenderScale());
    const float maximumScale = std::max(absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z));
    const float actorBoundsRadius = std::max(actor->GetRadius(), 1.0f) * maximumScale;
    const float maximumCasterDistance = shadowSettings.projectionHalfExtent * 1.6f + actorBoundsRadius;
    if (glm::distance(actor->GetRenderPosition(), focusPosition) > maximumCasterDistance) {
        return;
    }

    const std::vector<LoadedMesh>* meshes = actor->GetMeshes();
    DirectionalShadowShader* shadowShader = mDirectionalShadowMap->GetShader();
    if (!meshes || meshes->empty() || !shadowShader) {
        return;
    }

    const glm::mat4 model = CreateActorModelMatrix(actor, useOrient, 1.0f);
    glUniformMatrix4fv(shadowShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));

    const std::vector<glm::mat4>* skinningMatrices = actor->GetSkinningMatrices();
    const bool hasSkinningMatrices = skinningMatrices && !skinningMatrices->empty();
    if (hasSkinningMatrices) {
        const std::size_t uploadCount =
            std::min(skinningMatrices->size(), SkeletalAnimationConstants::MaxShaderBoneCount);
        glUniformMatrix4fv(shadowShader->GetLocBoneTransforms(), static_cast<GLsizei>(uploadCount), GL_FALSE,
                           glm::value_ptr(skinningMatrices->front()));
    }

    for (const LoadedMesh& mesh : *meshes) {
        const bool useSkinning = hasSkinningMatrices && mesh.hasBoneInfluences;
        glUniform1i(shadowShader->GetLocUseSkinning(), useSkinning ? 1 : 0);
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glUniform1i(shadowShader->GetLocUseSkinning(), 0);
}
