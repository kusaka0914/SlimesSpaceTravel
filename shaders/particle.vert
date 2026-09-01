#version 330 core

layout(location = 0) in vec2 localPosition;
layout(location = 1) in vec2 textureCoordinate;
layout(location = 2) in vec3 instancePosition;
layout(location = 3) in float instanceSize;
layout(location = 4) in vec4 instanceColor;
layout(location = 5) in float instanceRotation;
layout(location = 6) in float instanceStretch;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

out vec2 fragmentTextureCoordinate;
out vec4 fragmentColor;

void main()
{
    vec2 scaledPosition = localPosition;
    scaledPosition.x *= instanceStretch;

    float sineValue = sin(instanceRotation);
    float cosineValue = cos(instanceRotation);
    vec2 rotatedPosition = vec2(
        scaledPosition.x * cosineValue - scaledPosition.y * sineValue,
        scaledPosition.x * sineValue + scaledPosition.y * cosineValue
    );

    vec3 worldPosition = instancePosition +
                         cameraRight * rotatedPosition.x * instanceSize +
                         cameraUp * rotatedPosition.y * instanceSize;

    gl_Position = projection * view * vec4(worldPosition, 1.0);
    fragmentTextureCoordinate = textureCoordinate;
    fragmentColor = instanceColor;
}
