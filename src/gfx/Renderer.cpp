#include "Renderer.h"
#include "Game.h"
#include "VertexArray.h"
#include "thirdParty/stb_image.h"
#include <iostream>

Renderer::Renderer(Game* game)
    : mGame(game),
      mFont(nullptr)
{
    Initialize();
}

Renderer::~Renderer()
{
    if (mFont) {
        TTF_CloseFont(mFont);
    }
    TTF_Quit();
}

void Renderer::Initialize()
{
    InitializeFont();
    InitializeVertexArrays();
}

void Renderer::InitializeFont()
{
    if (TTF_Init() != 0) {
        // std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
    }

    mFont = TTF_OpenFont("../assets/fonts/NotoSansJP-Black.ttf", 72);
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
    int imgWidth, imgHeight, imgChannels;
    unsigned char* imgData = stbi_load(path.c_str(), &imgWidth, &imgHeight, &imgChannels, STBI_rgb_alpha);
    if (!imgData || imgWidth <= 0 || imgHeight <= 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(imgData);
        return;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
    stbi_image_free(imgData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const auto existing = mTextures.find(name);
    if (existing != mTextures.end()) {
        glDeleteTextures(1, &existing->second);
        existing->second = tex;
    } else {
        mTextures[name] = tex;
    }
}

GLuint Renderer::CreateTextTexture(const std::string& text, int& outWidth, int& outHeight, const SDL_Color textColor,
                                   float textScale) const
{
    outWidth = 0;
    outHeight = 0;

    if (!mFont) {
        return 0;
    }

    SDL_Surface* surf = TTF_RenderUTF8_Blended(mFont, text.c_str(), textColor);
    if (!surf) {
        return 0;
    }

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);

    if (!rgba) {
        return 0;
    }

    outWidth = rgba->w * textScale;
    outHeight = rgba->h * textScale;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(rgba);

    return tex;
}
