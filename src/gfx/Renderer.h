#pragma once

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Game;
class VertexArray;

class Renderer {
public:
    Renderer(Game* game);
    virtual ~Renderer();

protected:
    void RegisterTexture(const std::string& path, const std::string& name);

private:
    void Initialize();
    void InitializeFont();
    void InitializeVertexArrays();

protected:
    Game* mGame;
    TTF_Font* mFont;
    std::unordered_map<std::string, std::unique_ptr<VertexArray>> mVertexArrays;
    std::unordered_map<std::string, GLuint> mTextures;
};