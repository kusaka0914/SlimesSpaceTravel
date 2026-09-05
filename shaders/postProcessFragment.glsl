#version 330 core

out vec4 fragColor;

in vec2 texCoord;

uniform int renderPass;
uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform bool horizontalBlur;
uniform float bloomThreshold;
uniform float bloomSoftKnee;
uniform float bloomStrength;
uniform float exposure;

const int BRIGHT_EXTRACTION_PASS = 0;
const int BLUR_PASS = 1;
const int COMPOSITE_PASS = 2;

vec3 ExtractBrightColor(vec3 sceneColor)
{
    float brightness = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    float safeSoftKnee = max(bloomSoftKnee, 0.0001);
    float softContribution = clamp(
        (brightness - bloomThreshold + safeSoftKnee) /
            (2.0 * safeSoftKnee),
        0.0,
        1.0);
    softContribution = softContribution * softContribution;
    float contribution = max(
        brightness - bloomThreshold,
        softContribution * safeSoftKnee);
    contribution /= max(brightness, 0.0001);
    return sceneColor * max(contribution, 0.0);
}

vec3 ApplyGaussianBlur()
{
    const float weights[5] = float[](
        0.227027,
        0.1945946,
        0.1216216,
        0.054054,
        0.016216);
    vec2 texelSize = 1.0 / vec2(textureSize(sceneTexture, 0));
    vec2 sampleDirection = horizontalBlur
        ? vec2(texelSize.x, 0.0)
        : vec2(0.0, texelSize.y);
    vec3 blurredColor = texture(sceneTexture, texCoord).rgb * weights[0];
    for (int sampleIndex = 1; sampleIndex < 5; ++sampleIndex) {
        vec2 sampleOffset = sampleDirection * float(sampleIndex);
        blurredColor += texture(
            sceneTexture,
            texCoord + sampleOffset).rgb * weights[sampleIndex];
        blurredColor += texture(
            sceneTexture,
            texCoord - sampleOffset).rgb * weights[sampleIndex];
    }
    return blurredColor;
}

vec3 ApplyAcesToneMapping(vec3 hdrColor)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp(
        (hdrColor * (a * hdrColor + b)) /
            (hdrColor * (c * hdrColor + d) + e),
        0.0,
        1.0);
}

void main()
{
    if (renderPass == BRIGHT_EXTRACTION_PASS) {
        fragColor = vec4(
            ExtractBrightColor(texture(sceneTexture, texCoord).rgb),
            1.0);
        return;
    }
    if (renderPass == BLUR_PASS) {
        fragColor = vec4(ApplyGaussianBlur(), 1.0);
        return;
    }

    vec3 hdrColor = texture(sceneTexture, texCoord).rgb;
    hdrColor += texture(bloomTexture, texCoord).rgb * bloomStrength;
    vec3 toneMappedColor = ApplyAcesToneMapping(hdrColor * exposure);
    vec3 displayColor = pow(toneMappedColor, vec3(1.0 / 2.2));
    fragColor = vec4(displayColor, 1.0);
}
