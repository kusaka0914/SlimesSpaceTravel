#include "gfx/ui/UITextRenderer.h"

#include "gfx/TextRasterizer.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utility>

UITextRenderer::UITextRenderer(TTF_Font* font, const UIShader& shader, VertexArray& quad)
    : mFont(font),
      mShader(shader),
      mQuad(quad)
{
}

UITextRenderer::~UITextRenderer()
{
    Shutdown();
}

void UITextRenderer::DrawText(glm::ivec2 framebufferSize, float x, float y, float scale, const std::string& message,
                              bool isCenterBase, glm::vec4 color, float rotationDegrees, const TextEffect* effect)
{
    glUseProgram(mShader.GetShaderProgram());

    std::string firstLine = message;
    std::string secondLine;
    const bool hasSecondLine = SplitText(message, firstLine, secondLine);

    glm::vec2 rotationPivot(x, y);
    if (!isCenterBase) {
        int firstWidth = 0;
        int firstHeight = 0;
        int secondWidth = 0;
        int secondHeight = 0;
        TextRasterizer::Measure(mFont, firstLine, scale, firstWidth, firstHeight);
        if (hasSecondLine) {
            TextRasterizer::Measure(mFont, secondLine, scale, secondWidth, secondHeight);
        }

        const float textWidth = static_cast<float>(std::max(firstWidth, secondWidth));
        float textHeight = static_cast<float>(std::max(firstHeight, secondHeight));
        if (hasSecondLine) {
            textHeight += framebufferSize.y * 0.0666f;
        }
        rotationPivot += glm::vec2(textWidth, textHeight) * 0.5f;
    }

    const auto drawStyledLine = [&](const std::string& line, float lineOffset) {
        if (effect && effect->shadowEnabled && effect->shadowColor.a > 0.0f) {
            DrawTextLine(framebufferSize, line, x + effect->shadowOffset.x, y + effect->shadowOffset.y, scale,
                         isCenterBase, lineOffset, effect->shadowColor, rotationDegrees, rotationPivot);
        }

        if (effect && effect->outlineEnabled && effect->outlineWidth > 0.0f && effect->outlineColor.a > 0.0f) {
            DrawTextLine(framebufferSize, line, x, y, scale, isCenterBase, lineOffset, effect->outlineColor,
                         rotationDegrees, rotationPivot, effect->outlineWidth);
        }

        DrawTextLine(framebufferSize, line, x, y, scale, isCenterBase, lineOffset, color, rotationDegrees,
                     rotationPivot);
    };

    drawStyledLine(firstLine, hasSecondLine ? -framebufferSize.y * 0.0222f : 0.0f);

    if (!hasSecondLine) {
        return;
    }

    drawStyledLine(secondLine, framebufferSize.y * 0.0444f);
}

bool UITextRenderer::SplitText(const std::string& message, std::string& firstLine, std::string& secondLine)
{
    size_t newlinePosition = message.find('\n');
    if (newlinePosition != std::string::npos) {
        firstLine = message.substr(0, newlinePosition);
        secondLine = message.substr(newlinePosition + 1);
        return true;
    }

    newlinePosition = message.find("\\n");
    if (newlinePosition != std::string::npos) {
        firstLine = message.substr(0, newlinePosition);
        secondLine = message.substr(newlinePosition + 2);
        return true;
    }

    firstLine = message;
    secondLine.clear();
    return false;
}

void UITextRenderer::DrawTextLine(glm::ivec2 framebufferSize, const std::string& message, float x, float y, float scale,
                                  bool isCenterBase, float yOffset, glm::vec4 color, float rotationDegrees,
                                  glm::vec2 rotationPivot, float outlineWidth)
{
    const SDL_Color textColor{static_cast<Uint8>(color.x), static_cast<Uint8>(color.y), static_cast<Uint8>(color.z),
                              static_cast<Uint8>(color.w)};

    // 縁取り幅は画面上の寸法なので、フォント生成時は拡大前のピクセル数に戻す。
    const int outlinePixels =
        outlineWidth > 0.0f && scale > 0.0f ? std::max(1, static_cast<int>(std::round(outlineWidth / scale))) : 0;
    const float actualOutlineWidth = static_cast<float>(outlinePixels) * scale;
    const CachedTextTexture* cachedTexture = FindOrCreateTextTexture(message, textColor, outlinePixels);
    if (!cachedTexture) {
        return;
    }

    const int textWidth = static_cast<int>(static_cast<float>(cachedTexture->unscaledWidth) * scale);
    const int textHeight = static_cast<int>(static_cast<float>(cachedTexture->unscaledHeight) * scale);
    if (textWidth <= 0 || textHeight <= 0) {
        return;
    }

    glm::vec3 position;
    if (isCenterBase) {
        position = glm::vec3(x, y + yOffset, 0.0f);
    } else {
        position = glm::vec3(x + textWidth * 0.5f - actualOutlineWidth,
                             y + textHeight * 0.5f + yOffset - actualOutlineWidth, 0.0f);
    }

    if (rotationPivot == glm::vec2(0.0f)) {
        rotationPivot = glm::vec2(position.x, position.y);
    }

    const glm::vec3 pivot(rotationPivot.x, rotationPivot.y, 0.0f);
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), pivot) *
                            glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f)) *
                            glm::translate(glm::mat4(1.0f), position - pivot) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(textWidth, textHeight, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 projMat = glm::ortho(0.0f, static_cast<float>(framebufferSize.x),
                                         static_cast<float>(framebufferSize.y), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mShader.GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mShader.GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mShader.GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));
    glUniform1i(mShader.GetLocDiffuseTexture(), 0);
    glUniform1i(mShader.GetLocUseTexture(), 1);
    glUniform4f(mShader.GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cachedTexture->handle);

    mQuad.SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

std::size_t UITextRenderer::TextTextureCacheKeyHash::operator()(const TextTextureCacheKey& key) const
{
    const std::size_t textHash = std::hash<std::string>{}(key.text);
    const std::size_t colorHash = std::hash<std::uint32_t>{}(key.rgba);
    const std::size_t outlineHash = std::hash<int>{}(key.outlinePixels);
    return textHash ^ (colorHash << 1) ^ (outlineHash << 2);
}

const UITextRenderer::CachedTextTexture*
UITextRenderer::FindOrCreateTextTexture(const std::string& text, const SDL_Color& color, int outlinePixels)
{
    const std::uint32_t rgba = static_cast<std::uint32_t>(color.r) << 24 | static_cast<std::uint32_t>(color.g) << 16 |
                               static_cast<std::uint32_t>(color.b) << 8 | static_cast<std::uint32_t>(color.a);

    // 倍率をキャッシュキーから外し、同じ文字・色・縁取りのテクスチャをサイズ違いでも再利用する。
    const TextTextureCacheKey key{text, rgba, outlinePixels};

    const auto existingTexture = mTextTextureCache.find(key);
    if (existingTexture != mTextTextureCache.end()) {
        existingTexture->second.lastUseOrder = ++mNextTextTextureUseOrder;
        return &existingTexture->second;
    }

    constexpr std::size_t maxTextTextureCount = 256;
    if (mTextTextureCache.size() >= maxTextTextureCount) {
        EvictLeastRecentlyUsedTextTexture();
    }

    int unscaledWidth = 0;
    int unscaledHeight = 0;
    const GLuint textureHandle =
        TextRasterizer::CreateTexture(mFont, text, unscaledWidth, unscaledHeight, color, 1.0f, outlinePixels);
    if (textureHandle == 0 || unscaledWidth <= 0 || unscaledHeight <= 0) {
        return nullptr;
    }

    const auto [insertedTexture, wasInserted] = mTextTextureCache.emplace(
        key, CachedTextTexture{textureHandle, unscaledWidth, unscaledHeight, ++mNextTextTextureUseOrder});
    if (!wasInserted) {
        glDeleteTextures(1, &textureHandle);
    }
    return &insertedTexture->second;
}

void UITextRenderer::EvictLeastRecentlyUsedTextTexture()
{
    if (mTextTextureCache.empty()) {
        return;
    }

    const auto leastRecentlyUsedTexture =
        std::min_element(mTextTextureCache.begin(), mTextTextureCache.end(), [](const auto& left, const auto& right) {
            return left.second.lastUseOrder < right.second.lastUseOrder;
        });
    glDeleteTextures(1, &leastRecentlyUsedTexture->second.handle);
    mTextTextureCache.erase(leastRecentlyUsedTexture);
}

void UITextRenderer::Shutdown()
{
    for (const auto& cacheEntry : mTextTextureCache) {
        glDeleteTextures(1, &cacheEntry.second.handle);
    }
    mTextTextureCache.clear();
}

void UITextRenderer::DrawRubyText(glm::ivec2 framebufferSize, float x, float y, float scale, float rubyScaleRatio,
                                  float rubyGapRatio, const std::vector<RubyTextSegment>& segments, glm::vec4 color,
                                  bool centerBased, float rotationDegrees, float outlineWidth, glm::vec4 outlineColor)
{
    std::vector<std::vector<RubyTextSegment>> lines(1);

    for (const RubyTextSegment& sourceSegment : segments) {
        std::size_t position = 0;
        while (position < sourceSegment.text.size()) {
            const std::size_t actualNewline = sourceSegment.text.find('\n', position);
            const std::size_t escapedNewline = sourceSegment.text.find("\\n", position);

            std::size_t newlinePosition = std::string::npos;
            std::size_t delimiterLength = 0;
            if (actualNewline != std::string::npos &&
                (escapedNewline == std::string::npos || actualNewline < escapedNewline)) {
                newlinePosition = actualNewline;
                delimiterLength = 1;
            } else if (escapedNewline != std::string::npos) {
                newlinePosition = escapedNewline;
                delimiterLength = 2;
            }

            const std::size_t end = newlinePosition == std::string::npos ? sourceSegment.text.size() : newlinePosition;
            if (end > position) {
                RubyTextSegment part = sourceSegment;
                part.text = sourceSegment.text.substr(position, end - position);
                lines.back().emplace_back(std::move(part));
            }

            if (newlinePosition == std::string::npos) {
                break;
            }

            lines.emplace_back();
            position = newlinePosition + delimiterLength;
        }
    }

    if (lines.empty()) {
        return;
    }

    glUseProgram(mShader.GetShaderProgram());

    const float firstLineOffset = lines.size() > 1 ? -framebufferSize.y * 0.0222f : 0.0f;
    const float lineSpacing = framebufferSize.y * 0.0666f;

    float maximumLineWidth = 0.0f;
    float maximumBaseHeight = 0.0f;
    for (const auto& line : lines) {
        float lineWidth = 0.0f;
        for (const RubyTextSegment& segment : line) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (TextRasterizer::Measure(mFont, segment.text, scale, baseWidth, baseHeight)) {
                lineWidth += static_cast<float>(baseWidth);
                maximumBaseHeight = std::max(maximumBaseHeight, static_cast<float>(baseHeight));
            }
        }
        maximumLineWidth = std::max(maximumLineWidth, lineWidth);
    }
    const float totalTextHeight =
        maximumBaseHeight + (lines.size() > 1 ? lineSpacing * static_cast<float>(lines.size() - 1) : 0.0f);

    // 本文とルビが一体で回転するよう、文字列全体で共通の回転基準を使う。
    const glm::vec2 rotationPivot =
        centerBased ? glm::vec2(x, y) : glm::vec2(x + maximumLineWidth * 0.5f, y + totalTextHeight * 0.5f);

    const float baseStartX = centerBased ? x - maximumLineWidth * 0.5f : x;
    const float baseStartY = centerBased ? y - maximumBaseHeight * 0.5f : y;

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float currentX = baseStartX;
        const float lineY = baseStartY + firstLineOffset + lineSpacing * static_cast<float>(lineIndex);

        for (const RubyTextSegment& segment : lines[lineIndex]) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (!TextRasterizer::Measure(mFont, segment.text, scale, baseWidth, baseHeight)) {
                continue;
            }

            if (outlineWidth > 0.0f && outlineColor.a > 0.0f) {
                DrawTextLine(framebufferSize, segment.text, currentX, lineY, scale, false, 0.0f, outlineColor,
                             rotationDegrees, rotationPivot, outlineWidth);
            }
            DrawTextLine(framebufferSize, segment.text, currentX, lineY, scale, false, 0.0f, color, rotationDegrees,
                         rotationPivot);

            if (segment.showsRuby && !segment.reading.empty()) {
                const float rubyScale = scale * rubyScaleRatio;
                int rubyWidth = 0;
                int rubyHeight = 0;
                if (TextRasterizer::Measure(mFont, segment.reading, rubyScale, rubyWidth, rubyHeight)) {
                    const float rubyX = currentX + (static_cast<float>(baseWidth - rubyWidth) * 0.5f);
                    const float rubyY = lineY - static_cast<float>(rubyHeight) * (0.9f + rubyGapRatio);

                    // 小さいルビに縁取りを付けると文字内部が潰れて読みにくくなるため、縁取りしない。
                    DrawTextLine(framebufferSize, segment.reading, rubyX, rubyY, rubyScale, false, 0.0f, color,
                                 rotationDegrees, rotationPivot);
                }
            }

            currentX += static_cast<float>(baseWidth);
        }
    }
}
