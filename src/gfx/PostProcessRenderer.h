#pragma once

#include <GL/glew.h>
#include <memory>

class PostProcessShader;

// HDR画像の露出とBloomを調整する。blurIterationCountは水平方向・垂直方向のぼかしを1組とした回数。
struct PostProcessSettings {
    float exposure = 1.0f;
    float bloomThreshold = 1.05f;
    float bloomSoftKnee = 0.45f;
    float bloomStrength = 0.22f;
    int blurIterationCount = 5;
};

// HDRシーンの描画先とBloom用の中間画像を所有し、指定フレームバッファへ合成する。
class PostProcessRenderer {
public:
    PostProcessRenderer();
    ~PostProcessRenderer();

    // 正のピクセル寸法でHDR描画先を用意してバインドする。失敗時はfalseで、呼び出し側が通常描画へ切り替える。
    bool BeginScene(int width, int height);

    // BeginScene成功後の画像を合成する。描画先とviewportは指定値になり、深度・ブレンド等の有効状態は復元する。
    void CompositeTo(unsigned int destinationFramebuffer, int width, int height);

    // OpenGLコンテキストが有効な間に、所有する描画先・全画面形状・シェーダーを解放する。
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
