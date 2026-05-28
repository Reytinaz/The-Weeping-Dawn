#ifndef SHADER_HPP
#define SHADER_HPP

#include "matrix4.hpp"

class Shader {
private:
    unsigned int programID;

    std::string loadFile(const std::string& path);
    unsigned int compileShader(const std::string& source, unsigned int type);
    void checkCompileErrors(unsigned int shader, const std::string& type);

public:
    Shader();
    ~Shader();

    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragSrc);
    bool loadFromSource(const std::string& vertexSource, const std::string& geometrySource, const std::string& fragmentSource);
    bool loadComputeFromSource(const std::string& computeSrc);

    void use() const;
    void setUniform(const std::string& name, int value) const;
    void setUniform(const std::string& name, unsigned int value) const;
    void setUniform(const std::string& name, float value) const;
    void setUniform(const std::string& name, int x, int y);
    void setUniform(const std::string& name, const float* values, int count) const;
    void setUniform(const std::string& name, const Vector3& vec3) const;
    void setUniform(const std::string& name, const Vector2& vec2) const;
    void setUniform(const std::string& name, const Matrix4& mat) const;
    void setUniformTranspose(const std::string& name, const Matrix4& mat) const;
    void setUniform(const std::string& name, float x, float y, float z) const;

    unsigned int getProgramID() const { return programID; }
};
#endif