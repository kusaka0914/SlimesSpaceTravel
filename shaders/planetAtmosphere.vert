#version 330 core

// CPU側で拡大した大気用メッシュを変換し、視線・光源との計算に使うワールド座標と法線を渡す。

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

out vec3 worldPosition;
out vec3 worldNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 homogeneousWorldPosition = modelMatrix * vec4(vertexPosition, 1.0);

    worldPosition = homogeneousWorldPosition.xyz;

    // 非一様スケールでも面に垂直な法線を保つため、モデル行列の逆転置を使う。
    worldNormal = mat3(transpose(inverse(modelMatrix))) * vertexNormal;

    gl_Position = projectionMatrix * viewMatrix * homogeneousWorldPosition;
}
