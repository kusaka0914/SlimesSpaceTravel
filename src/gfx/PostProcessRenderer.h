#pragma once

#include <GL/glew.h>

#include <memory>

class PostProcessShader;

struct PostProcessSettings {
    float exposure = 1.0f;
    float bloomThreshold = 1.05f;
    float bloomSoftKnee = 0.45f;
    float bloomStrength = 0.22f;
    int blurIterationCount = 5;
};

class PostProcessRenderer {
public:
    PostProcessRenderer();
    ~PostProcessRenderer();

    bool BeginScene(int width, int height);
    void CompositeTo(unsigned int destinationFramebuffer, int width, int height);
    void Shutdown();

    PostProcessSettings& GetSettings() { return mSettings; }
    const PostProcessSettings& GetSettings() const { return mSettings; }

private:
    bool EnsureRenderTargets(int width, int height);
    bool CreateSceneTarget(int width, int height);
    bool CreateBloomTargets(int width, int height);
    void DrawFullscreenQuad() const;
    void DestroyRenderTargets();

    std::unique_ptr<PostProcessShader> mShader;
    PostProcessSettings mSettings;

    GLuint mSceneFramebuffer = 0;
    GLuint mSceneTexture = 0;
    GLuint mSceneDepthBuffer = 0;
    GLuint mBloomFramebuffers[2]{};
    GLuint mBloomTextures[2]{};
    GLuint mFullscreenVertexArray = 0;
    GLuint mFullscreenVertexBuffer = 0;
    int mWidth = 0;
    int mHeight = 0;
    int mBloomWidth = 0;
    int mBloomHeight = 0;
    bool mIsSceneActive = false;
};
