#version 330 core

// 惑星の輪郭を強調する大気の発光表現。描画側で出力RGBをアルファで重み付けして加算合成する。

in vec3 worldPosition;
in vec3 worldNormal;

out vec4 outputColor;

uniform vec3 cameraWorldPosition;
uniform vec3 atmosphereColor;
uniform float atmosphereStrength;
uniform float atmospherePower;

// 太陽光が進むワールド空間の方向。面から光源へ向かう方向は、この逆向きになる。
uniform vec3 sunDirection;

void main()
{
    vec3 surfaceNormal = normalize(worldNormal);
    vec3 viewDirection = normalize(cameraWorldPosition - worldPosition);
    vec3 lightDirection = normalize(-sunDirection);

    // 視線と法線が直交する輪郭ほど強く光らせる。absにより法線の表裏を同じ強さで扱う。
    float fresnel = pow(clamp(1.0 - abs(dot(surfaceNormal, viewDirection)), 0.0, 1.0), max(atmospherePower, 0.01));

    // 明暗境界を滑らかにつなぎ、夜側と側面に強めの光を与える。係数は見た目の調整値。
    float sunAlignment = dot(surfaceNormal, lightDirection);
    float nightAmount = 1.0 - smoothstep(-0.30, 0.30, sunAlignment);
    float sideLitAmount = 1.0 - abs(sunAlignment);
    float directionalStrength = mix(0.48, 0.92, nightAmount) + sideLitAmount * 0.14;
    float opacity = fresnel * atmosphereStrength * directionalStrength;

    // 夜側は青く、昼側は暖色に寄せる。輪郭に近いほど発光色も強める。
    vec3 nightColor = atmosphereColor * vec3(0.72, 0.90, 1.16);
    vec3 dayColor = atmosphereColor * vec3(1.08, 1.02, 0.92);
    vec3 glowColor = mix(dayColor, nightColor, nightAmount) * (0.45 + fresnel * 1.15);
    outputColor = vec4(glowColor, opacity);
}
