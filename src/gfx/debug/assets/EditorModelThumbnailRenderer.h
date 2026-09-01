#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

class Game;
struct LoadedModel;

class EditorModelThumbnailRenderer final {
public:
    explicit EditorModelThumbnailRenderer(Game* game);
    ~EditorModelThumbnailRenderer();

    EditorModelThumbnailRenderer(
        const EditorModelThumbnailRenderer&) = delete;
    EditorModelThumbnailRenderer& operator=(
        const EditorModelThumbnailRenderer&) = delete;

    void BeginFrame();
    GLuint ResolveThumbnail(const std::string& modelPath);
    GLuint ResolveThumbnail(
        const std::string& modelPath,
        const glm::vec3& modelScale,
        const std::string& textureOverridePath);
    bool HasFailed(const std::string& modelPath) const;
    void Clear();

private:
    bool GenerateThumbnail(
        const LoadedModel& loadedModel,
        const glm::vec3& modelScale,
        const std::string& textureOverridePath,
        GLuint& generatedTextureHandle);
    static std::string BuildThumbnailCacheKey(
        const std::string& modelPath,
        const glm::vec3& modelScale,
        const std::string& textureOverridePath);
    bool EnsureFramebuffer();

    Game* mGame = nullptr;
    GLuint mFramebuffer = 0;
    GLuint mDepthBuffer = 0;
    std::unordered_map<std::string, GLuint> mThumbnailTextures;
    std::unordered_set<std::string> mFailedModelPaths;
    bool mDidGenerateThumbnailThisFrame = false;
};
