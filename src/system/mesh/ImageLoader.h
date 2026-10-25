#pragma once

#include <memory>

// CPU上で画像をデコードする。デコーダーの反転設定を他の呼び出しへ公開しない。
namespace ImageLoader {

enum class RowOrder { TopToBottom, BottomToTop };

struct PixelDeleter {
    void operator()(unsigned char* pixels) const;
};

struct Image {
    std::unique_ptr<unsigned char, PixelDeleter> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
};

// 失敗時は空のpixels。channelsは0なら元画像、1〜4なら指定チャンネル数に変換する。
Image Load(const char* path, RowOrder rowOrder, int channels = 0);

} // namespace ImageLoader
