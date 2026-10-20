#pragma once

// エディターのゲーム画面・UGCプレビュー用のカラー／深度描画先を所有する。
// GLコンテキストを破棄する前にShutdownを呼ぶこと。
class GameRenderTargets {
public:
    GameRenderTargets() = default;
    ~GameRenderTargets();
    GameRenderTargets(const GameRenderTargets&) = delete;
    GameRenderTargets& operator=(const GameRenderTargets&) = delete;

    // 有効なピクセル寸法で描画先を確保する。同寸法なら再利用し、確保失敗時はその描画先を破棄する。
    // 不正寸法はfalseで既存描画先を保持する。呼出前のGL bindingとunpack設定を復元する。
    bool EnsureEditorGameTarget(int width, int height);
    bool EnsureUGCPreviewTarget(int width, int height);

    // 両方の描画先を解放し、ハンドルと寸法を0へ戻す。繰り返し呼び出せる。
    void Shutdown();

    unsigned int GetEditorGameFramebuffer() const;
    unsigned int GetEditorGameTexture() const;
    unsigned int GetUGCPreviewFramebuffer() const;
    unsigned int GetUGCPreviewTexture() const;
    int GetUGCPreviewWidth() const;
    int GetUGCPreviewHeight() const;

private:
    struct Target {
        unsigned int framebuffer = 0;
        unsigned int texture = 0;
        unsigned int depthBuffer = 0;
        int width = 0;
        int height = 0;
    };

    static bool EnsureTarget(Target& target, int width, int height);
    static void DestroyTarget(Target& target);

    Target mEditorGame;
    Target mUGCPreview;
};
