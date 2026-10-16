#version 330 core

// 粒子ごとのサイズ・伸長・回転を反映し、カメラに正対する板ポリゴンを配置する。

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexTextureCoordinate;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float instanceSize;
layout(location = 4) in vec4 instanceColor;
layout(location = 5) in float instanceRotation;
layout(location = 6) in float instanceStretch;

out vec2 textureCoordinate;
out vec4 fragmentColor;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

// カメラの右・上方向をワールド空間で受け取り、ビルボードの基底にする。
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main()
{
    // 板の横方向を伸ばしてから面内で回転する。instanceRotationの単位はラジアン。
    vec2 scaledPosition = vertexPosition;
    scaledPosition.x *= instanceStretch;

    float sineValue = sin(instanceRotation);
    float cosineValue = cos(instanceRotation);
    vec2 rotatedPosition = vec2(scaledPosition.x * cosineValue - scaledPosition.y * sineValue,
                                scaledPosition.x * sineValue + scaledPosition.y * cosineValue);

    vec3 worldPosition =
        instancePosition + cameraRight * rotatedPosition.x * instanceSize + cameraUp * rotatedPosition.y * instanceSize;

    gl_Position = projectionMatrix * viewMatrix * vec4(worldPosition, 1.0);
    textureCoordinate = vertexTextureCoordinate;
    fragmentColor = instanceColor;
}
