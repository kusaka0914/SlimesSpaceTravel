#include "gfx/Renderer.h"

#include "Game.h"
#include "gfx/VertexArray.h"
#include "thirdParty/stb_image.h"

#include <algorithm>
#include <iostream>

Renderer::Renderer(Game* game)
    : mGame(game),
      mFont(nullptr)
{
    Initialize();
}

Renderer::~Renderer()
{
    for (const auto& [textureName, texture] : mTextures) {
        glDeleteTextures(1, &texture);
    }
    if (mFont) {
        TTF_CloseFont(mFont);
    }
    if (mDidInitializeTtf) {
        TTF_Quit();
    }
}

void Renderer::Initialize()
{
    mIsInitialized = InitializeFont();
    InitializeVertexArrays();
}

bool Renderer::InitializeFont()
{
    if (TTF_Init() != 0) {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << '\n';
        return false;
    }
    mDidInitializeTtf = true;

    mFont = TTF_OpenFont("../assets/fonts/NotoSansJP-Black.ttf", 72);
    if (!mFont) {
        std::cerr << "Failed to open renderer font: " << TTF_GetError() << '\n';
        return false;
    }
    return true;
}

void Renderer::InitializeVertexArrays()
{
    const std::vector<float> quad = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    };
    mVertexArrays["quad"] = std::make_unique<VertexArray>(quad.data(), 4, nullptr, 0);

    const std::vector<float> quadFlipVertical = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    };
    mVertexArrays["quadFlipVertical"] = std::make_unique<VertexArray>(quadFlipVertical.data(), 4, nullptr, 0);

    const std::vector<float> hpBarQuad = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    };

    mVertexArrays["hpBar"] = std::make_unique<VertexArray>(hpBarQuad.data(), 4, nullptr, 0);
}

void Renderer::RegisterTexture(const std::string& path, const std::string& name)
{
    int imageWidth, imageHeight, channelCount;
    unsigned char* imageData = stbi_load(path.c_str(), &imageWidth, &imageHeight, &channelCount, STBI_rgb_alpha);
    if (!imageData || imageWidth <= 0 || imageHeight <= 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(imageData);
        return;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    stbi_image_free(imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const auto existing = mTextures.find(name);
    if (existing != mTextures.end()) {
        glDeleteTextures(1, &existing->second);
        existing->second = texture;
    } else {
        mTextures[name] = texture;
    }
}

GLuint Renderer::CreateTextTexture(const std::string& text, int& outWidth, int& outHeight, const SDL_Color textColor,
                                   float textScale, int outlinePixels) const
{
    outWidth = 0;
    outHeight = 0;

    if (!mFont) {
        return 0;
    }

    // フォントの縁取り設定は後続の文字描画にも影響するため、一時変更を描画直後に戻す。
    const int previousOutline = TTF_GetFontOutline(mFont);
    const int requestedOutline = std::max(outlinePixels, 0);
    if (previousOutline != requestedOutline) {
        TTF_SetFontOutline(mFont, requestedOutline);
    }

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(mFont, text.c_str(), textColor);
    if (previousOutline != requestedOutline) {
        TTF_SetFontOutline(mFont, previousOutline);
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

bool Renderer::MeasureText(const std::string& text, float textScale, int& outWidth, int& outHeight) const
{
    outWidth = 0;
    outHeight = 0;
    if (!mFont || text.empty()) {
        return false;
    }

    int unscaledWidth = 0;
    int unscaledHeight = 0;
    if (TTF_SizeUTF8(mFont, text.c_str(), &unscaledWidth, &unscaledHeight) != 0) {
        return false;
    }

    outWidth = static_cast<int>(unscaledWidth * textScale);
    outHeight = static_cast<int>(unscaledHeight * textScale);
    return outWidth > 0 && outHeight > 0;
}
