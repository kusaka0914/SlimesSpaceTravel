#version 330 core

const int MAX_BONE_COUNT = 128;
const int MAX_BONE_INFLUENCE_COUNT = 4;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIndices;
layout (location = 4) in vec4 aBoneWeights;
layout (location = 5) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useSkinning;
uniform bool useInstancing;
uniform mat4 boneTransforms[MAX_BONE_COUNT];

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out vec3 localFragPos;

void main()
{
    vec4 localPosition = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;

    if (useSkinning) {
        mat4 skinTransform = mat4(0.0);
        float totalBoneWeight = 0.0;

        for (int influenceIndex = 0;
             influenceIndex < MAX_BONE_INFLUENCE_COUNT;
             ++influenceIndex) {
            int boneIndex = aBoneIndices[influenceIndex];
            float boneWeight = aBoneWeights[influenceIndex];

            bool isBoneIndexValid =
                boneIndex >= 0 && boneIndex < MAX_BONE_COUNT;

            if (!isBoneIndexValid || boneWeight <= 0.0) {
                continue;
            }

            skinTransform += boneTransforms[boneIndex] * boneWeight;
            totalBoneWeight += boneWeight;
        }

        if (totalBoneWeight > 0.0) {
            skinTransform /= totalBoneWeight;

            localPosition = skinTransform * localPosition;
            localNormal = mat3(skinTransform) * localNormal;
        }
    }

    mat4 activeModel = useInstancing ? instanceModel : model;
    vec4 worldPosition = activeModel * localPosition;

    fragPos = worldPosition.xyz;
    normal = mat3(transpose(inverse(activeModel))) * localNormal;
    texCoord = aTexCoord;
    localFragPos = localPosition.xyz;

    gl_Position = projection * view * worldPosition;
}
