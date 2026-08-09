#pragma once

#include <GL/glew.h>

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
    bool HasFailed(const std::string& modelPath) const;
    void Clear();

private:
    bool GenerateThumbnail(
        const LoadedModel& loadedModel,
        GLuint& generatedTextureHandle);
    bool EnsureFramebuffer();

    Game* mGame = nullptr;
    GLuint mFramebuffer = 0;
    GLuint mDepthBuffer = 0;
    std::unordered_map<std::string, GLuint> mThumbnailTextures;
    std::unordered_set<std::string> mFailedModelPaths;
    bool mDidGenerateThumbnailThisFrame = false;
};
