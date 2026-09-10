#version 330 core

const int MAX_BONE_COUNT = 128;
const int MAX_BONE_INFLUENCE_COUNT = 4;

layout (location = 0) in vec3 aPos;
layout (location = 3) in ivec4 aBoneIndices;
layout (location = 4) in vec4 aBoneWeights;
layout (location = 5) in mat4 instanceModel;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;
uniform bool useInstancing;
uniform mat4 boneTransforms[MAX_BONE_COUNT];

void main()
{
    vec4 localPosition = vec4(aPos, 1.0);
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
            localPosition =
                (skinTransform / totalBoneWeight) * localPosition;
        }
    }
    mat4 activeModel = useInstancing ? instanceModel : model;
    gl_Position = lightSpaceMatrix * activeModel * localPosition;
}
