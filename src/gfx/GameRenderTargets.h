#pragma once

// エディターのゲーム画面・UGCプレビュー用のカラー／深度描画先を所有する。
// GLコンテキストを破棄する前にShutdownを呼ぶこと。
class GameRenderTargets {
public:
    // 正のピクセル寸法で描画先を確保する。同寸法なら再利用し、作成失敗時はその描画先を破棄してfalseを返す。
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
    void DestroyEditorGameTarget();
    void DestroyUGCPreviewTarget();

    unsigned int mEditorGameFramebuffer = 0;
    unsigned int mEditorGameTexture = 0;
    unsigned int mEditorGameDepthBuffer = 0;
    int mEditorGameWidth = 0;
    int mEditorGameHeight = 0;

    unsigned int mUGCPreviewFramebuffer = 0;
    unsigned int mUGCPreviewTexture = 0;
    unsigned int mUGCPreviewDepthBuffer = 0;
    int mUGCPreviewWidth = 0;
    int mUGCPreviewHeight = 0;
};
