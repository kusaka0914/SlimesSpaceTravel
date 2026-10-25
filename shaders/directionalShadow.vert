#version 330 core

// 通常描画と同じ頂点変形を行い、平行光源から見たクリップ座標へ変換する。

const int MAX_BONE_COUNT = 128;
const int MAX_BONE_INFLUENCE_COUNT = 4;

layout(location = 0) in vec3 vertexPosition;
layout(location = 3) in ivec4 vertexBoneIndices;
layout(location = 4) in vec4 vertexBoneWeights;
layout(location = 5) in mat4 instanceModelMatrix;

uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;
uniform bool useInstancing;
uniform mat4 boneTransforms[MAX_BONE_COUNT];

void main()
{
    vec4 skinnedPosition = vec4(vertexPosition, 1.0);

    // 影とモデルの形状が一致するよう、common.vertと同じボーン・重みの扱いにする。
    if (useSkinning) {
        mat4 skinTransform = mat4(0.0);
        float totalBoneWeight = 0.0;

        for (int influenceIndex = 0; influenceIndex < MAX_BONE_INFLUENCE_COUNT; ++influenceIndex) {
            int boneIndex = vertexBoneIndices[influenceIndex];
            float boneWeight = vertexBoneWeights[influenceIndex];

            bool isBoneIndexValid = boneIndex >= 0 && boneIndex < MAX_BONE_COUNT;

            if (!isBoneIndexValid || boneWeight <= 0.0) {
                continue;
            }

            skinTransform += boneTransforms[boneIndex] * boneWeight;
            totalBoneWeight += boneWeight;
        }

        // 有効な重みの合計で正規化する。重みがなければ元の頂点を保つ。
        if (totalBoneWeight > 0.0) {
            skinTransform /= totalBoneWeight;

            skinnedPosition = skinTransform * skinnedPosition;
        }
    }

    // ライトのビュー・射影行列はlightSpaceMatrixに合成済み。
    mat4 activeModelMatrix = useInstancing ? instanceModelMatrix : modelMatrix;
    gl_Position = lightSpaceMatrix * activeModelMatrix * skinnedPosition;
}
