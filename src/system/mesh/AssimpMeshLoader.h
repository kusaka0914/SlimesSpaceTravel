#pragma once

#include "system/mesh/LoadedModel.h"

class TextureLoader;

// Assimpからモデル・骨格・アニメーションを読み込み、描画用GPUメッシュを作成する。
class AssimpMeshLoader {
public:
    explicit AssimpMeshLoader(const TextureLoader* textureLoader);

    // 有効なOpenGLコンテキスト上で呼ぶ。nullptr・読込失敗時は空のモデルを返す。
    // 生成したGPUリソースは戻り値の破棄だけでは解放されない。
    LoadedModel LoadModelFromFile(const char* path) const;

private:
    const TextureLoader* mTextureLoader;
};
