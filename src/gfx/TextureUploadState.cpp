#include "gfx/TextureUploadState.h"

TextureUploadState::TextureUploadState()
{
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &mTexture);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &mUnpackBuffer);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &mAlignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &mRowLength);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &mSkipRows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &mSkipPixels);

    // PBOが残っていると、CPUポインターがバッファ内オフセットとして解釈される。
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
}

TextureUploadState::~TextureUploadState()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, mAlignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mRowLength);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, mSkipRows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, mSkipPixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mUnpackBuffer);
    glBindTexture(GL_TEXTURE_2D, mTexture == 0 || glIsTexture(mTexture) ? mTexture : 0);
}
