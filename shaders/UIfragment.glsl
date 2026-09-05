#version 330 core

out vec4 fragColor;
in vec2 texCoord;

uniform vec4 objectColor;
uniform int useTexture;
uniform sampler2D diffuseTexture;
uniform bool convertSrgbToLinear;

void main()
{
    vec4 baseColor = (useTexture != 0) ? texture(diffuseTexture, texCoord) : objectColor;
    if (useTexture != 0) {
        baseColor.a *= objectColor.a;
    }
    if (convertSrgbToLinear) {
        baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    }
    fragColor = baseColor;
}
