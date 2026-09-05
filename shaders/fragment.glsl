#version 330 core

out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec3 localFragPos;

uniform vec4 objectColor;
uniform int useTexture;
uniform sampler2D diffuseTexture;
uniform vec2 textureTiling;
uniform int useBackTexture;
uniform sampler2D backTexture;
uniform float textureSideBlendWidth;

uniform vec3 viewPos;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform vec3 environmentColor;
uniform float dayEnvironmentIntensity;
uniform float nightEnvironmentIntensity;
uniform vec3 rimColor;
uniform float dayRimStrength;
uniform float nightRimStrength;
uniform float rimPower;
uniform float materialMinimumReflectance;
uniform float materialRimBoost;
uniform bool isUnlit;
uniform float toonLevels;
uniform float toonStrength;
uniform vec3 colorMultiplier;
uniform bool applyOutputGamma;
uniform vec3 emissiveColor;
uniform float emissiveIntensity;
uniform mat4 lightSpaceMatrix;
uniform sampler2D shadowMap;
uniform bool shadowsEnabled;
uniform float shadowMinimumBias;
uniform float shadowMaximumBias;

float CalculateShadow(vec3 worldPosition, vec3 surfaceNormal, vec3 lightDirection)
{
    if (!shadowsEnabled) {
        return 0.0;
    }
    vec4 lightClipPosition = lightSpaceMatrix * vec4(worldPosition, 1.0);
    if (isnan(lightClipPosition.w) || isinf(lightClipPosition.w) ||
        abs(lightClipPosition.w) < 0.00001) {
        return 0.0;
    }

    vec3 projectedPosition = lightClipPosition.xyz / lightClipPosition.w;
    if (any(isnan(projectedPosition)) || any(isinf(projectedPosition))) {
        return 0.0;
    }

    projectedPosition = projectedPosition * 0.5 + 0.5;
    if (projectedPosition.z < 0.0 || projectedPosition.z > 1.0 ||
        projectedPosition.x < 0.0 || projectedPosition.x > 1.0 ||
        projectedPosition.y < 0.0 || projectedPosition.y > 1.0) {
        return 0.0;
    }

    float normalLightAlignment = max(dot(surfaceNormal, lightDirection), 0.0);
    float bias = max(
        shadowMaximumBias * (1.0 - normalLightAlignment),
        shadowMinimumBias);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    for (int offsetY = -1; offsetY <= 1; ++offsetY) {
        for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            vec2 sampleCoordinates = projectedPosition.xy +
                vec2(offsetX, offsetY) * texelSize;
            if (sampleCoordinates.x < 0.0 || sampleCoordinates.x > 1.0 ||
                sampleCoordinates.y < 0.0 || sampleCoordinates.y > 1.0) {
                continue;
            }

            float closestDepth = texture(shadowMap, sampleCoordinates).r;
            if (isnan(closestDepth) || isinf(closestDepth)) {
                continue;
            }
            shadow += projectedPosition.z - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    return clamp(shadow / 9.0, 0.0, 1.0);
}

void main()
{
    vec4 baseColor = (useTexture != 0) ? texture(diffuseTexture, texCoord * textureTiling) : objectColor;
    if (useTexture != 0) {
        baseColor.a *= objectColor.a;
    }
    if (useBackTexture != 0) {
        vec4 backColor = texture(backTexture, texCoord * textureTiling);
        float blendWidth = max(textureSideBlendWidth, 0.00001);
        float backAmount = smoothstep(
            -blendWidth,
            blendWidth,
            -localFragPos.y);
        baseColor = mix(baseColor, backColor, backAmount);
    }

    if (isUnlit) {
        fragColor = baseColor;
        return;
    }

    float normalLength = length(normal);
    vec3 norm = normalLength > 0.00001
        ? normal / normalLength
        : vec3(0.0, 1.0, 0.0);
    vec3 lightDir = normalize(-sunDirection);
    vec3 viewDir = normalize(viewPos - fragPos);

    float sunAlignment = dot(norm, lightDir);
    float nightAmount = 1.0 - smoothstep(-0.30, 0.28, sunAlignment);
    float environmentIntensity = mix(
        dayEnvironmentIntensity,
        nightEnvironmentIntensity,
        nightAmount);
    vec3 environmentLight = environmentColor * environmentIntensity;

    float diff = max(sunAlignment, 0.0);
    float levels = max(toonLevels, 1.0);
    float toonDiffuse = floor(diff * levels) / levels;
    float finalDiffuse = mix(diff, toonDiffuse, toonStrength);
    vec3 diffuseLight = finalDiffuse * sunColor * sunIntensity;
    float shadow = CalculateShadow(fragPos, norm, lightDir);

    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, rimPower);
    float rimStrength = mix(
        dayRimStrength,
        nightRimStrength,
        nightAmount) + materialRimBoost;
    vec3 rimLight = rimStrength * rim * rimColor;

    vec3 lighting = environmentLight +
        diffuseLight * (1.0 - shadow) + rimLight;
    vec3 visibleBaseColor = max(
        baseColor.rgb,
        vec3(materialMinimumReflectance));
    vec3 finalColor = visibleBaseColor * lighting;
    finalColor += baseColor.rgb * emissiveColor * emissiveIntensity;
    finalColor *= colorMultiplier;
    if (applyOutputGamma) {
        finalColor = pow(
            clamp(finalColor, 0.0, 1.0),
            vec3(1.0 / 2.2));
    }

    fragColor = vec4(finalColor, baseColor.a);
}
