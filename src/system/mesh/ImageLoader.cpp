#include "system/mesh/ImageLoader.h"

// STBの設定とAPIをこの翻訳単位に閉じ、読込ごとに行順を指定する。
#define STB_IMAGE_STATIC
#define STBIDEF [[maybe_unused]] static
#define STB_IMAGE_IMPLEMENTATION
#include "thirdParty/stb_image.h"

namespace ImageLoader {

void PixelDeleter::operator()(unsigned char* pixels) const
{
    stbi_image_free(pixels);
}

Image Load(const char* path, RowOrder rowOrder, int channels)
{
    if (!path || channels < 0 || channels > 4) {
        return {};
    }
    stbi_set_flip_vertically_on_load_thread(rowOrder == RowOrder::BottomToTop);
    Image image;
    int sourceChannels = 0;
    image.pixels.reset(stbi_load(path, &image.width, &image.height, &sourceChannels, channels));
    if (!image.pixels) {
        return {};
    }
    image.channels = channels == 0 ? sourceChannels : channels;
    return image;
}

} // namespace ImageLoader
