#version 330 core

// 単色・テクスチャ共通のUI描画。テクスチャ使用時も描画色のRGBとアルファを掛け合わせる。

in vec2 textureCoordinate;

out vec4 outputColor;

uniform vec4 objectColor;
uniform bool useTexture;
uniform sampler2D diffuseTexture;
uniform bool convertSrgbToLinear;
uniform bool grayscale;
uniform vec2 panelSize;
uniform float cornerRadius;
uniform float borderWidth;
uniform vec4 borderColor;

void main()
{
    vec4 baseColor = useTexture ? texture(diffuseTexture, textureCoordinate) * objectColor : objectColor;

    if (useTexture && grayscale) {
        // 青いジュエルや盾でも陰影を残すため、最大成分を明度として無彩色にする。
        baseColor.rgb = vec3(max(baseColor.r, max(baseColor.g, baseColor.b)));
    }

    if (!useTexture && (cornerRadius > 0.0 || borderWidth > 0.0)) {
        // 矩形の内側への符号付き距離で角を丸め、縁の1ピクセルを滑らかにする。
        vec2 distanceToCorner = abs((textureCoordinate - 0.5) * panelSize) - panelSize * 0.5 + cornerRadius;
        float edgeDistance = length(max(distanceToCorner, vec2(0.0))) +
                             min(max(distanceToCorner.x, distanceToCorner.y), 0.0) - cornerRadius;
        if (borderWidth > 0.0) {
            float borderCoverage = smoothstep(-borderWidth - 0.5, -borderWidth + 0.5, edgeDistance);
            float borderAlpha = borderColor.a * borderCoverage;
            // 枠線を背景色へ重ね、半透明背景の濃さを枠線の内側でも保つ。
            float combinedAlpha = borderAlpha + baseColor.a * (1.0 - borderAlpha);
            baseColor.rgb = (borderColor.rgb * borderAlpha + baseColor.rgb * baseColor.a * (1.0 - borderAlpha)) /
                            max(combinedAlpha, 0.000001);
            baseColor.a = combinedAlpha;
        }
        baseColor.a *= 1.0 - smoothstep(-0.5, 0.5, edgeDistance);
    }

    // リニア空間の描画先へ合成する場合は、RGBをガンマ2.2で近似変換する。
    // アルファは色空間変換の対象にしない。
    if (convertSrgbToLinear) {
        baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    }

    outputColor = baseColor;
}
