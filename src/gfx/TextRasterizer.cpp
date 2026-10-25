#include "gfx/TextRasterizer.h"

#include <algorithm>

GLuint TextRasterizer::CreateTexture(TTF_Font* font, const std::string& text, int& outWidth, int& outHeight,
                                     const SDL_Color textColor, float textScale, int outlinePixels)
{
    outWidth = 0;
    outHeight = 0;

    if (!font) {
        return 0;
    }

    // フォントの縁取り設定は後続の文字描画にも影響するため、一時変更を描画直後に戻す。
    const int previousOutline = TTF_GetFontOutline(font);
    const int requestedOutline = std::max(outlinePixels, 0);
    if (previousOutline != requestedOutline) {
        TTF_SetFontOutline(font, requestedOutline);
    }

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text.c_str(), textColor);
    if (previousOutline != requestedOutline) {
        TTF_SetFontOutline(font, previousOutline);
    }

    if (!textSurface) {
        return 0;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(textSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(textSurface);

    if (!rgbaSurface) {
        return 0;
    }

    outWidth = rgbaSurface->w * textScale;
    outHeight = rgbaSurface->h * textScale;

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rgbaSurface->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(rgbaSurface);

    return texture;
}

bool TextRasterizer::Measure(TTF_Font* font, const std::string& text, float textScale, int& outWidth, int& outHeight)
{
    outWidth = 0;
    outHeight = 0;
    if (!font || text.empty()) {
        return false;
    }

    int unscaledWidth = 0;
    int unscaledHeight = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &unscaledWidth, &unscaledHeight) != 0) {
        return false;
    }

    outWidth = static_cast<int>(unscaledWidth * textScale);
    outHeight = static_cast<int>(unscaledHeight * textScale);
    return outWidth > 0 && outHeight > 0;
}
