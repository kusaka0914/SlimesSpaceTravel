#pragma once

#include "Renderer.h"
#include "text/RubyText.h"

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Actor;
class DebugLabelRenderer;
class Enemy;
class Game;
class NPCProximityMessageRenderer;
class Player;
class PlayerEffectRenderer;
class ParticleRenderer;
class Planet;
class RenderViewportController;
class SceneObjectRenderer;
class Shader3D;
class VertexArray;

class Renderer3D : public Renderer {
public:
    explicit Renderer3D(Game* game);
    ~Renderer3D();

    void Initialize();
    void Draw() const;

    void DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos) const;

    Game* GetGame() const { return mGame; }
    Shader3D* GetShader3D() const { return mShader3D; }
    std::unordered_map<std::string, std::unique_ptr<VertexArray>>& GetVertexArrays() { return mVertexArrays; }
    std::unordered_map<std::string, GLuint>& GetTextures() { return mTextures; }
    GLuint GetAttackRangeVAO() const { return mAttackRangeVAO; }
    GLuint GetAttackRangeVBO() const { return mAttackRangeVBO; }
    GLuint GetOrLoadTextureOverride(const std::string& assetRelativePath);

    GLuint CreateTextTextureFor3D(const std::string& text, int& outWidth, int& outHeight, const SDL_Color textColor,
                                  float textScale) const
    {
        return CreateTextTexture(text, outWidth, outHeight, textColor, textScale);
    }
    GLuint CreateRubyTextTextureFor3D(
        const std::vector<RubyTextSegment>& segments,
        int& outWidth,
        int& outHeight,
        int& outBaseTextHeight,
        const SDL_Color textColor,
        float rubyScaleRatio,
        float rubyGapRatio) const;

    void StartTransparentDraw() const;
    void EndTransparentDraw() const;

    void TryDrawActor(Actor* actor, bool useOrient = true) const;
    void DrawActor(Actor* actor, bool useOrient = true) const;

    template <class ActorType> void TryDrawActors(const std::vector<ActorType*>& actors, bool useOrient = true) const
    {
        if (actors.empty()) {
            return;
        }

        for (ActorType* actor : actors) {
            if (!actor || !actor->GetIsActive() ||
                actor->GetRenderOpacity() <= 0.001f) {
                continue;
            }

            DrawActor(actor, useOrient);
        }
    }

    void DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode, const glm::vec4& color) const;

private:
    void InitializeAttackRangeBuffer();
    void InitializeRenderModules();

    void SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos) const;
    glm::mat4 CreateActorModelMatrix(Actor* actor, bool useOrient, float scaleMultiplier = 1.0f) const;
    void DrawActorSelectionOutline(Actor* actor, bool useOrient) const;

    bool UploadActorSkinningMatrices(const Actor* actor) const;
    void SetSkinningEnabled(bool isEnabled) const;

private:
    std::unique_ptr<Shader3D> mShader3DUnique;
    Shader3D* mShader3D;

    std::unique_ptr<RenderViewportController> mRenderViewportController;
    std::unique_ptr<SceneObjectRenderer> mSceneObjectRenderer;
    std::unique_ptr<PlayerEffectRenderer> mPlayerEffectRenderer;
    std::unique_ptr<DebugLabelRenderer> mDebugLabelRenderer;
    std::unique_ptr<NPCProximityMessageRenderer> mNPCProximityMessageRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    GLuint mAttackRangeVAO;
    GLuint mAttackRangeVBO;
    std::unordered_set<std::string> mFailedTextureOverrides;
};
