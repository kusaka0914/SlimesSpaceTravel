#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

class Game;
struct LoadedModel;

// モデルのプレビュー画像を生成し、GLテクスチャ・フレームバッファ・深度バッファを所有する。
// Gameは非所有参照。生成・キャッシュ解放・破棄は有効なGLコンテキスト上で行う。
class EditorModelThumbnailRenderer final {
public:
    explicit EditorModelThumbnailRenderer(Game* game);
    ~EditorModelThumbnailRenderer();

    EditorModelThumbnailRenderer(const EditorModelThumbnailRenderer&) = delete;
    EditorModelThumbnailRenderer& operator=(const EditorModelThumbnailRenderer&) = delete;

    // フレームの先頭で呼び、新規生成の制限を解除する。キャッシュは保持する。
    void BeginFrame();

    // モデル・スケール・差し替え画像に対応する借用ハンドルを返す。新規生成は1フレームに1件まで試みる。
    // 生成失敗・生成待ちは0。返したハンドルはClearまたは破棄まで有効で、呼び出し側では解放しない。
    GLuint ResolveThumbnail(const std::string& modelPath);
    GLuint ResolveThumbnail(const std::string& modelPath, const glm::vec3& modelScale,
                            const std::string& textureOverridePath);

    // 標準スケール・差し替え画像なしの生成失敗を調べる。別の外観の失敗は含まない。
    bool HasFailed(const std::string& modelPath) const;

    // キャッシュ画像と失敗履歴を消去し、再試行を許可する。フレームバッファと深度バッファは保持する。
    void Clear();

private:
    bool GenerateThumbnail(const LoadedModel& loadedModel, const glm::vec3& modelScale,
                           const std::string& textureOverridePath, GLuint& outTextureHandle);
    static std::string BuildThumbnailCacheKey(const std::string& modelPath, const glm::vec3& modelScale,
                                              const std::string& textureOverridePath);
    bool EnsureFramebuffer();

    Game* mGame = nullptr;
    GLuint mFramebuffer = 0;
    GLuint mDepthBuffer = 0;
    std::unordered_map<std::string, GLuint> mThumbnailTextures;
    std::unordered_set<std::string> mFailedThumbnailKeys;
    bool mDidGenerateThumbnailThisFrame = false;
};
