#version 330 core

// 画面全体を覆うポリゴン用。入力位置は既に正規化デバイス座標のため、行列変換を行わない。

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexTextureCoordinate;

out vec2 textureCoordinate;

void main()
{
    textureCoordinate = vertexTextureCoordinate;
    gl_Position = vec4(vertexPosition, 0.0, 1.0);
}
