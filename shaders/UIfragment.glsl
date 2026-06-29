#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform vec4 objectColor;
uniform int useTexture;
uniform sampler2D diffuseTexture;

void main()
{
    vec4 baseColor = (useTexture != 0) ? texture(diffuseTexture, texCoord) : objectColor;
    fragColor = baseColor;
}