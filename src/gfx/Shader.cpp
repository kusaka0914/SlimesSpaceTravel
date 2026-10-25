#include "gfx/Shader.h"

#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader()
    : mShaderProgram(0),
      mLocModel(-1),
      mLocView(-1),
      mLocProj(-1),
      mLocObjectColor(-1),
      mLocUseTexture(-1),
      mLocDiffuseTexture(-1)
{
}

Shader::~Shader()
{
    glDeleteProgram(mShaderProgram);
}

std::string Shader::ReadShaderSource(const std::string& path) const
{
    const std::ifstream file(path);

    if (!file.is_open()) {
        return "";
    }

    std::stringstream sourceStream;
    sourceStream << file.rdbuf();
    return sourceStream.str();
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) const
{
    const unsigned int shaderId = glCreateShader(type);
    const char* sourceData = source.c_str();

    glShaderSource(shaderId, 1, &sourceData, nullptr);
    glCompileShader(shaderId);

    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[2048];
        glGetShaderInfoLog(shaderId, sizeof(log), nullptr, log);

        std::cerr << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << " shader compile error:\n" << log << '\n';

        glDeleteShader(shaderId);
        return 0;
    }
    return shaderId;
}

unsigned int Shader::CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) const
{
    const std::string vertexSource = ReadShaderSource(vertexPath);
    const std::string fragmentSource = ReadShaderSource(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        return 0;
    };

    const unsigned int vertexId = CompileShader(GL_VERTEX_SHADER, vertexSource);
    const unsigned int fragmentId = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!vertexId || !fragmentId) {
        // 片方だけコンパイルに成功した場合も、リンクせずに残ったオブジェクトを解放する。
        if (vertexId != 0) {
            glDeleteShader(vertexId);
        }
        if (fragmentId != 0) {
            glDeleteShader(fragmentId);
        }
        return 0;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexId);
    glAttachShader(program, fragmentId);
    glLinkProgram(program);

    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);

        std::cerr << "Shader program link error:\n" << log << '\n';

        glDeleteProgram(program);
        return 0;
    }
    return program;
}
