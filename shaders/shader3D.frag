#version 330 core

// テクスチャと太陽光・環境光・輪郭光を合成する、3Dモデル用のシェーディング。

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 textureCoordinate;
in vec3 localPosition;

out vec4 outputColor;

// ベース色と、ローカルY座標で切り替える表裏のテクスチャ。
uniform vec4 objectColor;
uniform bool useTexture;
uniform sampler2D diffuseTexture;
uniform vec2 textureTiling;
uniform bool useBackTexture;
uniform sampler2D backTexture;
uniform float textureSideBlendWidth;

// カメラ位置と光源方向はワールド空間。sunDirectionは太陽光が進む向き。
uniform vec3 cameraWorldPosition;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform vec3 environmentColor;
uniform float dayEnvironmentIntensity;
uniform float nightEnvironmentIntensity;

// 輪郭光と、暗い材質でも形状を読み取れるようにする補正。
uniform vec3 rimColor;
uniform float dayRimStrength;
uniform float nightRimStrength;
uniform float rimPower;
uniform float materialMinimumReflectance;
uniform float materialRimBoost;
uniform float materialDarkSurfaceLightBoost;

// 描画モード、色調整、発光。
uniform bool isUnlit;
uniform float toonLevels;
uniform float toonStrength;
uniform vec3 colorMultiplier;
uniform float colorSaturation;
uniform bool applyOutputGamma;
uniform vec3 emissiveColor;
uniform float emissiveIntensity;

// 非操作側の分裂Playerだけに適用する硬化表現。通常のActorでは0にする。
uniform float hardeningAmount;

// 平行光源のシャドウマップと、深度比較の誤差を吸収するバイアス。
uniform mat4 lightSpaceMatrix;
uniform sampler2D shadowMap;
uniform bool useShadows;
uniform float shadowMinimumBias;
uniform float shadowMaximumBias;

// 影の割合を0（遮蔽なし）～1（完全に遮蔽）で返す。影は太陽の直接光だけに適用する。
float CalculateShadow(vec3 worldPosition, vec3 surfaceNormal, vec3 lightDirection)
{
    if (!useShadows) {
        return 0.0;
    }
    vec4 lightClipPosition = lightSpaceMatrix * vec4(worldPosition, 1.0);
    if (isnan(lightClipPosition.w) || isinf(lightClipPosition.w) || abs(lightClipPosition.w) < 0.00001) {
        return 0.0;
    }

    vec3 projectedPosition = lightClipPosition.xyz / lightClipPosition.w;
    if (any(isnan(projectedPosition)) || any(isinf(projectedPosition))) {
        return 0.0;
    }

    // 透視除算後の座標を[-1, 1]から[0, 1]へ変換する。マップ範囲外は遮蔽なしとする。
    projectedPosition = projectedPosition * 0.5 + 0.5;
    if (projectedPosition.z < 0.0 || projectedPosition.z > 1.0 || projectedPosition.x < 0.0 ||
        projectedPosition.x > 1.0 || projectedPosition.y < 0.0 || projectedPosition.y > 1.0) {
        return 0.0;
    }

    // 光に対して傾いた面ほどバイアスを増やし、自己遮蔽による縞を抑える。
    float normalLightAlignment = max(dot(surfaceNormal, lightDirection), 0.0);
    float bias = max(shadowMaximumBias * (1.0 - normalLightAlignment), shadowMinimumBias);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    // 周囲3×3点の深度比較を平均して影の境界を和らげる（PCF）。
    // 範囲外・不正な深度の点は遮蔽なしとして数え、分母は9に固定する。
    for (int offsetY = -1; offsetY <= 1; ++offsetY) {
        for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            vec2 sampleTextureCoordinate = projectedPosition.xy + vec2(offsetX, offsetY) * texelSize;
            if (sampleTextureCoordinate.x < 0.0 || sampleTextureCoordinate.x > 1.0 || sampleTextureCoordinate.y < 0.0 ||
                sampleTextureCoordinate.y > 1.0) {
                continue;
            }

            float closestDepth = texture(shadowMap, sampleTextureCoordinate).r;
            if (isnan(closestDepth) || isinf(closestDepth)) {
                continue;
            }
            shadow += projectedPosition.z - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    return clamp(shadow / 9.0, 0.0, 1.0);
}

vec4 CalculateBaseColor()
{
    vec4 baseColor = useTexture ? texture(diffuseTexture, textureCoordinate * textureTiling) : objectColor;
    if (useTexture) {
        baseColor.a *= objectColor.a;
    }

    // 面の表裏判定ではなくローカルYの符号で選ぶ。負側を裏の色とし、境界だけ滑らかに混ぜる。
    if (useBackTexture) {
        vec4 backColor = texture(backTexture, textureCoordinate * textureTiling);
        float blendWidth = max(textureSideBlendWidth, 0.00001);
        float backAmount = smoothstep(-blendWidth, blendWidth, -localPosition.y);
        baseColor = mix(baseColor, backColor, backAmount);
    }

    return baseColor;
}

vec3 CalculateLighting(vec3 baseColor)
{
    // 長さ0の法線で除算しないよう、代わりにワールド上向きを使う。
    float normalLength = length(worldNormal);
    vec3 surfaceNormal = normalLength > 0.00001 ? worldNormal / normalLength : vec3(0.0, 1.0, 0.0);
    vec3 lightDirection = normalize(-sunDirection);
    vec3 viewDirection = normalize(cameraWorldPosition - worldPosition);

    // 太陽との角度から昼夜を滑らかに切り替え、直接光が届かない面にも環境光を残す。
    float sunAlignment = dot(surfaceNormal, lightDirection);
    float nightAmount = 1.0 - smoothstep(-0.30, 0.28, sunAlignment);
    float environmentIntensity = mix(dayEnvironmentIntensity, nightEnvironmentIntensity, nightAmount);
    vec3 environmentLight = environmentColor * environmentIntensity;

    // 拡散光を段階化し、toonStrengthで通常の陰影とトゥーン調の陰影を混ぜる。
    float diffuseAmount = max(sunAlignment, 0.0);
    float toonLevelCount = max(toonLevels, 1.0);
    float toonDiffuse = floor(diffuseAmount * toonLevelCount) / toonLevelCount;
    float finalDiffuse = mix(diffuseAmount, toonDiffuse, toonStrength);
    vec3 diffuseLight = finalDiffuse * sunColor * sunIntensity;
    float shadow = CalculateShadow(worldPosition, surfaceNormal, lightDirection);

    // 輪郭ほど強くなるリム光。丸め誤差でpowの底が負になり、NaNが発生しないよう制限する。
    float rim = clamp(1.0 - dot(viewDirection, surfaceNormal), 0.0, 1.0);
    rim = pow(rim, max(rimPower, 0.01));
    float rimStrength = mix(dayRimStrength, nightRimStrength, nightAmount) + materialRimBoost;
    vec3 rimLight = rimStrength * rim * rimColor;

    // 暗い材質には環境色の補助光を足し、影の中でも形状を読み取りやすくする。
    float baseLuminance = dot(clamp(baseColor, 0.0, 1.0), vec3(0.2126, 0.7152, 0.0722));
    float darkSurfaceAmount = 1.0 - smoothstep(0.20, 0.70, baseLuminance);
    vec3 darkSurfaceLight = environmentColor * materialDarkSurfaceLightBoost * darkSurfaceAmount;

    vec3 lighting = environmentLight + diffuseLight * (1.0 - shadow) + rimLight + darkSurfaceLight;

    return lighting;
}

vec3 ApplyColorAdjustments(vec3 finalColor)
{
    finalColor *= colorMultiplier;
    float luminance = dot(finalColor, vec3(0.2126, 0.7152, 0.0722));
    finalColor = mix(vec3(luminance), finalColor, clamp(colorSaturation, 0.0, 1.0));

    // HDR出力をポストプロセスへ渡す場合は無効にし、表示用のガンマ補正を二重に掛けない。
    if (applyOutputGamma) {
        finalColor = pow(clamp(finalColor, 0.0, 1.0), vec3(1.0 / 2.2));
    }

    return finalColor;
}

void main()
{
    vec4 baseColor = CalculateBaseColor();

    // 非ライティング描画は、照明・発光・後段の色補正を通さずベース色を返す。
    if (isUnlit) {
        outputColor = baseColor;
        return;
    }

    vec3 lighting = CalculateLighting(baseColor.rgb);

    // 反射に使う色へ下限を設け、黒い材質が照明を完全に失わないようにする。
    vec3 visibleBaseColor = max(baseColor.rgb, vec3(materialMinimumReflectance));

    // 硬化した体は陰影の境界を少し明確にする。形状やベース色は変えない。
    if (hardeningAmount > 0.0) {
        vec3 hardenedLighting = floor(lighting * 6.0 + 0.5) / 6.0;
        lighting = mix(lighting, hardenedLighting, hardeningAmount * 0.6);
    }
    vec3 finalColor = visibleBaseColor * lighting;
    finalColor += baseColor.rgb * emissiveColor * emissiveIntensity;

    finalColor = ApplyColorAdjustments(finalColor);

    outputColor = vec4(finalColor, baseColor.a);
}
