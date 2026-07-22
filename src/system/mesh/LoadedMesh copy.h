#pragma once

struct LoadedMesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int indexCount = 0;
    unsigned int textureID = 0;
    float diffuseColor[3] = {1.0f, 1.0f, 1.0f};
    bool hasBoneInfluences = false;
};
