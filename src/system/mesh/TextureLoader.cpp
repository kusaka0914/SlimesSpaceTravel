#include "system/mesh/TextureLoader.h"

#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdParty/stb_image.h"

unsigned int TextureLoader::LoadTexture(const char* path, TextureColorSpace colorSpace) const
{
    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width = 0;
    int height = 0;
    int channelCount = 0;

    stbi_set_flip_vertically_on_load(1);
    unsigned char* pixels = stbi_load(path, &width, &height, &channelCount, 0);
    if (!pixels) {
        glDeleteTextures(1, &textureId);
        return 0;
    }

    GLenum format = GL_RGBA;
    if (channelCount == 1) {
        format = GL_RED;
    } else if (channelCount == 2) {
        format = GL_RG;
    } else if (channelCount == 3) {
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

    // 行のバイト数が4の倍数でない画像でも、ピクセル列を正しく転送する。
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}
