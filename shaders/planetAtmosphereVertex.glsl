#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPosition;
out vec3 worldNormal;

void main()
{
    vec4 transformedPosition = model * vec4(aPos, 1.0);
    worldPosition = transformedPosition.xyz;
    worldNormal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * transformedPosition;
}
