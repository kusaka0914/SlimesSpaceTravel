#include <GL/glew.h>

#include "gfx/GameRenderTargets.h"

bool GameRenderTargets::EnsureEditorGameTarget(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (mEditorGameFramebuffer == 0) {
        glGenFramebuffers(1, &mEditorGameFramebuffer);
        glGenTextures(1, &mEditorGameTexture);
        glGenRenderbuffers(1, &mEditorGameDepthBuffer);
    }

    if (width == mEditorGameWidth && height == mEditorGameHeight) {
        return true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mEditorGameFramebuffer);
    glBindTexture(GL_TEXTURE_2D, mEditorGameTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        mEditorGameTexture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, mEditorGameDepthBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, mEditorGameDepthBuffer);

    const bool isComplete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!isComplete) {
        DestroyEditorGameTarget();
        return false;
    }

    mEditorGameWidth = width;
    mEditorGameHeight = height;
    return true;
}

bool GameRenderTargets::EnsureUGCPreviewTarget(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (mUGCPreviewFramebuffer == 0) {
        glGenFramebuffers(1, &mUGCPreviewFramebuffer);
        glGenTextures(1, &mUGCPreviewTexture);
        glGenRenderbuffers(1, &mUGCPreviewDepthBuffer);
    }

    if (width == mUGCPreviewWidth && height == mUGCPreviewHeight) {
        return true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mUGCPreviewFramebuffer);
    glBindTexture(GL_TEXTURE_2D, mUGCPreviewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        mUGCPreviewTexture, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, mUGCPreviewDepthBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, mUGCPreviewDepthBuffer);

    const bool isComplete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!isComplete) {
        DestroyUGCPreviewTarget();
        return false;
    }

    mUGCPreviewWidth = width;
    mUGCPreviewHeight = height;
    return true;
}

void GameRenderTargets::Shutdown()
{
    DestroyEditorGameTarget();
    DestroyUGCPreviewTarget();
}

unsigned int GameRenderTargets::GetEditorGameFramebuffer() const
{
    return mEditorGameFramebuffer;
}

unsigned int GameRenderTargets::GetEditorGameTexture() const
{
    return mEditorGameTexture;
}

unsigned int GameRenderTargets::GetUGCPreviewFramebuffer() const
{
    return mUGCPreviewFramebuffer;
}

unsigned int GameRenderTargets::GetUGCPreviewTexture() const
{
    return mUGCPreviewTexture;
}

int GameRenderTargets::GetUGCPreviewWidth() const
{
    return mUGCPreviewWidth;
}

int GameRenderTargets::GetUGCPreviewHeight() const
{
    return mUGCPreviewHeight;
}

void GameRenderTargets::DestroyEditorGameTarget()
{
    if (mEditorGameDepthBuffer != 0) {
        glDeleteRenderbuffers(1, &mEditorGameDepthBuffer);
    }
    if (mEditorGameTexture != 0) {
        glDeleteTextures(1, &mEditorGameTexture);
    }
    if (mEditorGameFramebuffer != 0) {
        glDeleteFramebuffers(1, &mEditorGameFramebuffer);
    }

    mEditorGameDepthBuffer = 0;
    mEditorGameTexture = 0;
    mEditorGameFramebuffer = 0;
    mEditorGameWidth = 0;
    mEditorGameHeight = 0;
}

void GameRenderTargets::DestroyUGCPreviewTarget()
{
    if (mUGCPreviewDepthBuffer != 0) {
        glDeleteRenderbuffers(1, &mUGCPreviewDepthBuffer);
    }
    if (mUGCPreviewTexture != 0) {
        glDeleteTextures(1, &mUGCPreviewTexture);
    }
    if (mUGCPreviewFramebuffer != 0) {
        glDeleteFramebuffers(1, &mUGCPreviewFramebuffer);
    }

    mUGCPreviewDepthBuffer = 0;
    mUGCPreviewTexture = 0;
    mUGCPreviewFramebuffer = 0;
    mUGCPreviewWidth = 0;
    mUGCPreviewHeight = 0;
}
