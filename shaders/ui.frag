#version 330 core

// 単色・テクスチャ共通のUI描画。テクスチャ使用時も描画色のRGBとアルファを掛け合わせる。

in vec2 textureCoordinate;

out vec4 outputColor;

uniform vec4 objectColor;
uniform bool useTexture;
uniform sampler2D diffuseTexture;
uniform bool convertSrgbToLinear;
uniform bool grayscale;

void main()
{
    vec4 baseColor = useTexture ? texture(diffuseTexture, textureCoordinate) * objectColor : objectColor;

    if (useTexture && grayscale) {
        // 青いジュエルや盾でも陰影を残すため、最大成分を明度として無彩色にする。
        baseColor.rgb = vec3(max(baseColor.r, max(baseColor.g, baseColor.b)));
    }

    // リニア空間の描画先へ合成する場合は、RGBをガンマ2.2で近似変換する。
    // アルファは色空間変換の対象にしない。
    if (convertSrgbToLinear) {
        baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    }

    outputColor = baseColor;
}
