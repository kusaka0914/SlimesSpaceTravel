#pragma once

// 位置3・法線3・UV2のfloat列をGPUへコピーし、VAO・頂点バッファ・インデックスバッファを所有する。
// 生成・破棄時にはOpenGLコンテキストが必要。入力配列を生成後も保持する必要はない。
class VertexArray {
public:
    VertexArray(const float* vertices, unsigned int vertexCount, const unsigned int* indices, unsigned int indexCount);
    ~VertexArray();
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    // このVAOを現在の描画用にバインドする。描画命令自体は呼び出し側で発行する。
    void SetActive();

private:
    unsigned int mVertexBuffer;
    unsigned int mIndexBuffer;
    unsigned int mVertexArray;
};
