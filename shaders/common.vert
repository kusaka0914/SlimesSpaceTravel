#version 330 core

// 3DとUIで共用する頂点変換。必要に応じてスキニングとインスタンシングを適用する。

const int MAX_BONE_COUNT = 128;
const int MAX_BONE_INFLUENCE_COUNT = 4;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTextureCoordinate;
layout(location = 3) in ivec4 vertexBoneIndices;
layout(location = 4) in vec4 vertexBoneWeights;
layout(location = 5) in mat4 instanceModelMatrix;

out vec3 worldPosition;
out vec3 worldNormal;
out vec2 textureCoordinate;

// スキニング後・モデル変換前の位置。3D描画の表裏テクスチャの切り替えに使う。
out vec3 localPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform bool useSkinning;
uniform bool useInstancing;
uniform mat4 boneTransforms[MAX_BONE_COUNT];

void main()
{
    vec4 skinnedPosition = vec4(vertexPosition, 1.0);
    vec3 skinnedNormal = vertexNormal;

    // ボーン変換を重み付きで合成し、バインドポーズの頂点と法線を現在のポーズへ変形する。
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

        // 有効な重みの合計で正規化する。重みがなければ元の頂点と法線を保つ。
        if (totalBoneWeight > 0.0) {
            skinTransform /= totalBoneWeight;

            skinnedPosition = skinTransform * skinnedPosition;
            skinnedNormal = mat3(skinTransform) * skinnedNormal;
        }
    }

    // インスタンス描画と通常描画で、モデル行列の取得元を切り替える。
    mat4 activeModelMatrix = useInstancing ? instanceModelMatrix : modelMatrix;
    vec4 homogeneousWorldPosition = activeModelMatrix * skinnedPosition;

    worldPosition = homogeneousWorldPosition.xyz;

    // 非一様スケールでも面に垂直な法線を保つため、モデル行列の逆転置を使う。
    worldNormal = mat3(transpose(inverse(activeModelMatrix))) * skinnedNormal;

    textureCoordinate = vertexTextureCoordinate;
    localPosition = skinnedPosition.xyz;

    gl_Position = projectionMatrix * viewMatrix * homogeneousWorldPosition;
}
