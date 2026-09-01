#pragma once

class GameRenderTargets {
public:
    bool EnsureEditorGameTarget(int width, int height);
    bool EnsureUGCPreviewTarget(int width, int height);
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
