#include "gfx/GameRenderTargets.h"

#include "gfx/TextureUploadState.h"

#include <algorithm>

GameRenderTargets::~GameRenderTargets()
{
    Shutdown();
}

bool GameRenderTargets::EnsureEditorGameTarget(int width, int height)
{
    return EnsureTarget(mEditorGame, width, height);
}

bool GameRenderTargets::EnsureUGCPreviewTarget(int width, int height)
{
    return EnsureTarget(mUGCPreview, width, height);
}

bool GameRenderTargets::EnsureTarget(Target& target, int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (target.framebuffer != 0 && width == target.width && height == target.height) {
        return true;
    }
    GLint maxTextureSize = 0;
    GLint maxRenderbufferSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    const int maxSize = std::min(maxTextureSize, maxRenderbufferSize);
    if (width > maxSize || height > maxSize) {
        return false;
    }

    GLint drawFramebuffer = 0;
    GLint readFramebuffer = 0;
    GLint renderbuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
    TextureUploadState textureState;
    if (target.framebuffer == 0) {
        glGenFramebuffers(1, &target.framebuffer);
        glGenTextures(1, &target.texture);
        glGenRenderbuffers(1, &target.depthBuffer);
    }

    bool isComplete = false;
    if (target.framebuffer != 0 && target.texture != 0 && target.depthBuffer != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glBindTexture(GL_TEXTURE_2D, target.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, target.depthBuffer);
        GLint textureWidth = 0;
        GLint textureHeight = 0;
        GLint depthWidth = 0;
        GLint depthHeight = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &depthWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &depthHeight);
        // 再確保の失敗時に旧ストレージが残っても、旧寸法のFBOを成功扱いにしない。
        isComplete = textureWidth == width && textureHeight == height && depthWidth == width && depthHeight == height &&
                     glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    if (isComplete) {
        target.width = width;
        target.height = height;
    } else {
        DestroyTarget(target);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                      drawFramebuffer == 0 || glIsFramebuffer(drawFramebuffer) ? drawFramebuffer : 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                      readFramebuffer == 0 || glIsFramebuffer(readFramebuffer) ? readFramebuffer : 0);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer == 0 || glIsRenderbuffer(renderbuffer) ? renderbuffer : 0);
    return isComplete;
}

void GameRenderTargets::Shutdown()
{
    DestroyTarget(mEditorGame);
    DestroyTarget(mUGCPreview);
}

unsigned int GameRenderTargets::GetEditorGameFramebuffer() const
{
    return mEditorGame.framebuffer;
}

unsigned int GameRenderTargets::GetEditorGameTexture() const
{
    return mEditorGame.texture;
}

unsigned int GameRenderTargets::GetUGCPreviewFramebuffer() const
{
    return mUGCPreview.framebuffer;
}

unsigned int GameRenderTargets::GetUGCPreviewTexture() const
{
    return mUGCPreview.texture;
}

int GameRenderTargets::GetUGCPreviewWidth() const
{
    return mUGCPreview.width;
}

int GameRenderTargets::GetUGCPreviewHeight() const
{
    return mUGCPreview.height;
}

void GameRenderTargets::DestroyTarget(Target& target)
{
    if (target.depthBuffer != 0) {
        glDeleteRenderbuffers(1, &target.depthBuffer);
    }
    if (target.texture != 0) {
        glDeleteTextures(1, &target.texture);
    }
    if (target.framebuffer != 0) {
        glDeleteFramebuffers(1, &target.framebuffer);
    }
    target = {};
}
