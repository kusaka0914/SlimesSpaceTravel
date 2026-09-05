#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

out vec4 fragColor;

uniform vec3 viewPosition;
uniform vec3 atmosphereColor;
uniform float atmosphereStrength;
uniform float atmospherePower;
uniform vec3 sunDirection;

void main()
{
    vec3 normal = normalize(worldNormal);
    vec3 viewDirection = normalize(viewPosition - worldPosition);
    vec3 lightDirection = normalize(-sunDirection);
    float fresnel = pow(
        1.0 - abs(dot(normal, viewDirection)),
        max(atmospherePower, 0.01));
    float sunAlignment = dot(normal, lightDirection);
    float nightAmount = 1.0 - smoothstep(-0.30, 0.30, sunAlignment);
    float sideLitAmount = 1.0 - abs(sunAlignment);
    float directionalStrength = mix(0.48, 0.92, nightAmount) +
        sideLitAmount * 0.14;
    float opacity = fresnel * atmosphereStrength * directionalStrength;
    vec3 nightColor = atmosphereColor * vec3(0.72, 0.90, 1.16);
    vec3 dayColor = atmosphereColor * vec3(1.08, 1.02, 0.92);
    vec3 glowColor = mix(dayColor, nightColor, nightAmount) *
        (0.45 + fresnel * 1.15);
    fragColor = vec4(glowColor, opacity);
}
