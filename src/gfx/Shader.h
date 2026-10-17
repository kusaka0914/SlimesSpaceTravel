#pragma once

#include <string>

// GLSLの読み込み・コンパイル・リンクと、mShaderProgramの解放を担う。
// 生成したGLリソースを扱う間は、対応するOpenGLコンテキストを有効に保つこと。
class Shader {
public:
    Shader();
    ~Shader();

    unsigned int GetShaderProgram() const { return mShaderProgram; }
    int GetLocModel() const { return mLocModel; }
    int GetLocView() const { return mLocView; }
    int GetLocProj() const { return mLocProj; }
    int GetLocObjectColor() const { return mLocObjectColor; }
    int GetLocUseTexture() const { return mLocUseTexture; }
    int GetLocDiffuseTexture() const { return mLocDiffuseTexture; }

protected:
    // 読み込み・コンパイル・リンクの失敗時は0。成功したプログラムはmShaderProgramへ保持して管理する。
    unsigned int CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) const;

private:
    std::string ReadShaderSource(const std::string& path) const;
    unsigned int CompileShader(unsigned int type, const std::string& source) const;

protected:
    unsigned int mShaderProgram;
    int mLocModel;
    int mLocView;
    int mLocProj;
    int mLocObjectColor;
    int mLocUseTexture;
    int mLocDiffuseTexture;
};
