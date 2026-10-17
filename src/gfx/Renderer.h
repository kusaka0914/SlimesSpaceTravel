#pragma once

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Game;
class VertexArray;

// UI・3D描画で使うフォント、基本形状、テクスチャ生成処理の共通基盤。Gameは非所有参照。
// 生成時にSDL_ttfとGLリソースを初期化し、破棄時に解放する。GameとGLコンテキストはRendererより長く有効に保つ。
class Renderer {
public:
    Renderer(Game* game);
    virtual ~Renderer();

    // フォントと派生Rendererのシェーダーが初期化できた場合にtrue。失敗時の終了処理はGame側が担当する。
    bool IsInitialized() const { return mIsInitialized; }

protected:
    // 名前付きテクスチャを登録する。同名は旧テクスチャを解放して置換し、読込失敗時は既存の登録を維持する。
    void RegisterTexture(const std::string& path, const std::string& name);

    // UTF-8文字列のテクスチャを生成する。戻り値は呼び出し側が解放し、失敗時は0。出力寸法は倍率適用後のピクセル数。
    GLuint CreateTextTexture(const std::string& text, int& outWidth, int& outHeight, const SDL_Color textColor,
                             float textScale, int outlinePixels = 0) const;

    // 描画せずに倍率適用後のピクセル寸法を求める。空文字・フォント未設定・計測失敗・非正の寸法ならfalse。
    bool MeasureText(const std::string& text, float textScale, int& outWidth, int& outHeight) const;

private:
    void Initialize();
    bool InitializeFont();
    void InitializeVertexArrays();

protected:
    Game* mGame;
    TTF_Font* mFont;
    bool mDidInitializeTtf = false;
    bool mIsInitialized = false;
    std::unordered_map<std::string, std::unique_ptr<VertexArray>> mVertexArrays;
    mutable std::unordered_map<std::string, GLuint> mTextures;
};
