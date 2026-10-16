#pragma once

#include <cstddef>

namespace SkeletalAnimationConstants {
// MeshVertexと頂点シェーダーのボーン添字・重みの要素数に合わせる。
inline constexpr std::size_t MaxBoneInfluencesPerVertex = 4;

// モデル読込・行列転送の上限。スキニング対応シェーダーのMAX_BONE_COUNTと一致させる。
inline constexpr std::size_t MaxShaderBoneCount = 128;
}
