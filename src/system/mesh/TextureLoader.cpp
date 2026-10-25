#include "system/mesh/TextureLoader.h"

#include "gfx/TextureUploadState.h"
#include "system/mesh/ImageLoader.h"

#include <GL/glew.h>

unsigned int TextureLoader::LoadTexture(const char* path, TextureColorSpace colorSpace) const
{
    const auto image = ImageLoader::Load(path, ImageLoader::RowOrder::BottomToTop);
    if (!image.pixels) {
        return 0;
    }
    const TextureUploadState state;
    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGBA;
    if (image.channels == 1) {
        format = GL_RED;
    } else if (image.channels == 2) {
        format = GL_RG;
    } else if (image.channels == 3) {
        format = GL_RGB;
    }
    GLenum internalFormat = format;
    if (colorSpace == TextureColorSpace::SRGB) {
        if (format == GL_RGB) {
            internalFormat = GL_SRGB8;
        } else if (format == GL_RGBA) {
            internalFormat = GL_SRGB8_ALPHA8;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, GL_UNSIGNED_BYTE,
                 image.pixels.get());
    glGenerateMipmap(GL_TEXTURE_2D);

    return textureId;
}
