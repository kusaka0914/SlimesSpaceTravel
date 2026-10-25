#version 330 core

// 粒子のテクスチャにインスタンスの色と透明度を掛け合わせる。

in vec2 textureCoordinate;
in vec4 fragmentColor;

out vec4 outputColor;

uniform sampler2D particleTexture;

void main()
{
    vec4 textureColor = texture(particleTexture, textureCoordinate);
    vec4 baseColor = textureColor * fragmentColor;

    // ほぼ透明な断片を除外し、不要な色の合成を省く。
    if (baseColor.a <= 0.003) {
        discard;
    }

    outputColor = baseColor;
}
