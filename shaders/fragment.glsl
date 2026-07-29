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
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float rimStrength;
uniform float rimPower;
uniform float toonLevels;
uniform float toonStrength;

void main()
{
    vec4 baseColor = (useTexture != 0) ? texture(diffuseTexture, texCoord * textureTiling) : objectColor;
    if (useBackTexture != 0) {
        vec4 backColor = texture(backTexture, texCoord * textureTiling);
        float blendWidth = max(textureSideBlendWidth, 0.00001);
        float backAmount = smoothstep(
            -blendWidth,
            blendWidth,
            -localFragPos.y);
        baseColor = mix(baseColor, backColor, backAmount);
    }

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 ambient = ambientStrength * lightColor;

    float diff = max(dot(norm, lightDir), 0.0);
    float levels = max(toonLevels, 1.0);
    float toonDiffuse = floor(diff * levels) / levels;
    float finalDiffuse = mix(diff, toonDiffuse, toonStrength);
    vec3 diffuseLight = finalDiffuse * lightColor;

    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, rimPower);
    vec3 rimLight = rimStrength * rim * lightColor;

    vec3 lighting = ambient + diffuseLight + rimLight;
    vec3 finalColor = baseColor.rgb * lighting;
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    fragColor = vec4(finalColor, baseColor.a);
}
