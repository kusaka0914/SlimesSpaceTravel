#include "gfx/Renderer.h"

#include "gfx/TextRasterizer.h"
#include "gfx/VertexArray.h"
#include "thirdParty/stb_image.h"

#include <iostream>

Renderer::Renderer()
    : mFont(nullptr)
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
    return TextRasterizer::CreateTexture(mFont, text, outWidth, outHeight, textColor, textScale, outlinePixels);
}

bool Renderer::MeasureText(const std::string& text, float textScale, int& outWidth, int& outHeight) const
{
    return TextRasterizer::Measure(mFont, text, textScale, outWidth, outHeight);
}
