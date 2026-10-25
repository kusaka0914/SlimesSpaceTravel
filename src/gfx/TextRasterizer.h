#pragma once

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <string>

// 借用フォントの計測と、2D・3D描画で共用する文字テクスチャの生成処理。
namespace TextRasterizer {
// UTF-8文字列を描画用テクスチャにする。失敗時は0。戻り値のGLテクスチャは呼び出し側が解放する。
// 出力寸法は倍率適用後のピクセル数。生成には有効なGLコンテキストが必要。
GLuint CreateTexture(TTF_Font* font, const std::string& text, int& outWidth, int& outHeight, SDL_Color textColor,
                     float textScale, int outlinePixels = 0);

// 倍率適用後のピクセル寸法を求める。空文字・フォント未設定・計測失敗・非正の寸法ならfalse。
bool Measure(TTF_Font* font, const std::string& text, float textScale, int& outWidth, int& outHeight);
} // namespace TextRasterizer
