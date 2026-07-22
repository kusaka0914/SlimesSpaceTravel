#include "Shader.h"
#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <sstream>

Shader::Shader() {}

Shader::~Shader()
{
    glDeleteProgram(mShaderProgram);
}

std::string Shader::GetShaderSrcFromFile(const std::string& path) const
{
    const std::ifstream file(path);

    if (!file.is_open()) {
        // std::cerr << "Cannot open file: " << path << std::endl;
        return "";
    }

    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) const
{
    const unsigned int id = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[2048];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);

        std::cerr << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << " shader compile error:\n" << log << '\n';

        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int Shader::CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) const
{
    const std::string vertexSrc = GetShaderSrcFromFile(vertexPath);
    const std::string fragmentSrc = GetShaderSrcFromFile(fragmentPath);

    if (vertexSrc.empty() || fragmentSrc.empty()) {
        return 0;
    };

    const unsigned int vertexId = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    const unsigned int fragmentId = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    if (!vertexId || !fragmentId) {
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