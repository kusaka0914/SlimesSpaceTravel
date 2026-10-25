#pragma once

#include "text/RubyText.h"

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class UIShader;
class VertexArray;

// 文字・ルビ・影・縁取りを描画し、文字テクスチャキャッシュを所有する。
// フォント・シェーダー・quadは借用し、このインスタンスより長く有効に保つ。描画・解放にはGLコンテキストが必要。
class UITextRenderer {
public:
    // 文字の影・縁取り設定。色は0〜255のRGBA、幅とオフセットは画面上のピクセル数。
    struct TextEffect {
        bool shadowEnabled = false;
        glm::vec2 shadowOffset = glm::vec2(0.0f);
        glm::vec4 shadowColor = glm::vec4(0.0f);
        bool outlineEnabled = false;
        float outlineWidth = 0.0f;
        glm::vec4 outlineColor = glm::vec4(0.0f);
    };

    UITextRenderer(TTF_Font* font, const UIShader& shader, VertexArray& quad);
    ~UITextRenderer();

    UITextRenderer(const UITextRenderer&) = delete;
    UITextRenderer& operator=(const UITextRenderer&) = delete;

    // framebufferSizeは描画先のピクセル寸法。座標は左上原点、回転は度、色は0〜255のRGBA。
    // isCenterBaseで基準点を中心／左上に切り替える。effectはこの呼び出し中だけ借用する。
    void DrawText(glm::ivec2 framebufferSize, float x, float y, float scale, const std::string& message,
                  bool isCenterBase, glm::vec4 color, float rotationDegrees, const TextEffect* effect = nullptr);

    // 本文とルビを共通の基準で回転させて描く。座標・寸法・色の単位はDrawTextと同じ。
    // rubyScaleRatioは本文に対する文字倍率、rubyGapRatioはルビの高さを基準にした上下間隔の補正。
    void DrawRubyText(glm::ivec2 framebufferSize, float x, float y, float scale, float rubyScaleRatio,
                      float rubyGapRatio, const std::vector<RubyTextSegment>& segments, glm::vec4 color,
                      bool centerBased, float rotationDegrees, float outlineWidth, glm::vec4 outlineColor);

    // 最初の改行で2つに分ける。実改行を優先し、なければ文字列中の\nを使う。改行なしならfalse。
    static bool SplitText(const std::string& message, std::string& firstLine, std::string& secondLine);

    // キャッシュ内のGLテクスチャを解放する。繰り返し呼べ、以後の描画時には必要な文字を再生成する。
    void Shutdown();

private:
    struct TextTextureCacheKey {
        std::string text;
        std::uint32_t rgba = 0;
        int outlinePixels = 0;

        bool operator==(const TextTextureCacheKey& other) const
        {
            return text == other.text && rgba == other.rgba && outlinePixels == other.outlinePixels;
        }
    };

    struct TextTextureCacheKeyHash {
        std::size_t operator()(const TextTextureCacheKey& key) const;
    };

    struct CachedTextTexture {
        GLuint handle = 0;
        int unscaledWidth = 0;
        int unscaledHeight = 0;
        std::uint64_t lastUseOrder = 0;
    };

    const CachedTextTexture* FindOrCreateTextTexture(const std::string& text, const SDL_Color& color,
                                                     int outlinePixels);
    void EvictLeastRecentlyUsedTextTexture();
    void DrawTextLine(glm::ivec2 framebufferSize, const std::string& message, float x, float y, float scale,
                      bool isCenterBase, float yOffset, glm::vec4 color, float rotationDegrees, glm::vec2 rotationPivot,
                      float outlineWidth = 0.0f);

private:
    TTF_Font* mFont;
    const UIShader& mShader;
    VertexArray& mQuad;
    std::unordered_map<TextTextureCacheKey, CachedTextTexture, TextTextureCacheKeyHash> mTextTextureCache;
    std::uint64_t mNextTextTextureUseOrder = 0;
};
