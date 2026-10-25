#pragma once

#include <string>

// 派生ShaderのGLSLコンパイル・リンクとプログラム解放を共通化する。GLコンテキストはShaderより長く有効に保つ。
// プログラムは単独所有とし、基底型での生成・破棄、コピー・ムーブは許可しない。
class Shader {
public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    unsigned int GetShaderProgram() const { return mShaderProgram; }
    int GetLocModel() const { return mLocModel; }
    int GetLocView() const { return mLocView; }
    int GetLocProj() const { return mLocProj; }
    int GetLocObjectColor() const { return mLocObjectColor; }
    int GetLocUseTexture() const { return mLocUseTexture; }
    int GetLocDiffuseTexture() const { return mLocDiffuseTexture; }

protected:
    Shader();
    ~Shader();

    // 読み込み・コンパイル・リンクの失敗時は0。成功時のハンドルは呼び出し側でmShaderProgramへ代入して所有する。
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
