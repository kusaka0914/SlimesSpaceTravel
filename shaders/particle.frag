#version 330 core

in vec2 fragmentTextureCoordinate;
in vec4 fragmentColor;

uniform sampler2D particleTexture;

out vec4 outputColor;

void main()
{
    vec4 textureColor = texture(particleTexture, fragmentTextureCoordinate);
    vec4 color = textureColor * fragmentColor;

    if (color.a <= 0.003) {
        discard;
    }

    outputColor = color;
}
