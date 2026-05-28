#include "shader.hpp"
#include "glad.h"
#include "fstream"
#include "filesystem"
#include "sstream"

namespace {
    std::filesystem::path resourcesDir() {
        return "";
    }
}

Shader::Shader() : programID(0) {}

Shader::~Shader() {
    if (programID) {
        glDeleteProgram(programID);
    }
}

std::string Shader::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open shader file: " << path << std::endl;
        return "";
    }

    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    return stream.str();
}

unsigned int Shader::compileShader(const std::string& source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");

    return shader;
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

bool Shader::loadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = loadFile(vertexPath);
    std::string fragmentCode = loadFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return false;
    }

    return loadFromSource(vertexCode, fragmentCode);
}

bool Shader::loadFromSource(const std::string& vertexSrc, const std::string& fragSrc) {
    unsigned int vertexShader = compileShader(vertexSrc, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragSrc, GL_FRAGMENT_SHADER);

    if (!vertexShader || !fragmentShader) {
        return false;
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    checkCompileErrors(programID, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& geometrySource, const std::string& fragmentSource) {
    if (programID) glDeleteProgram(programID);

    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint geometryShader = compileShader(geometrySource, GL_GEOMETRY_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    if (!vertexShader || !geometryShader || !fragmentShader) {
        return false;
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, geometryShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programID, 512, NULL, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);

    return true;
}

bool Shader::loadComputeFromSource(const std::string& computeSrc) {
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = computeSrc.c_str();
    glShaderSource(computeShader, 1, &src, NULL);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        std::cerr << "Compute shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(computeShader);
        return false;
    }

    programID = glCreateProgram();
    glAttachShader(programID, computeShader);
    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programID, 512, NULL, infoLog);
        std::cerr << "Compute program linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(programID);
        programID = 0;
        glDeleteShader(computeShader);
        return false;
    }
    glDeleteShader(computeShader);
    return true;
}

void Shader::use() const {
    glUseProgram(programID);
}

void Shader::setUniform(const std::string& name, unsigned int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setUniform(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setUniform(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setUniform(const std::string& name, int x, int y) {
    glUniform2i(glGetUniformLocation(programID, name.c_str()), x, y);
}

void Shader::setUniform(const std::string& name, const float* values, int count) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), count / 16, GL_FALSE, values);
}

void Shader::setUniform(const std::string& name, const Vector3& vec3) const {
    glUniform3f(glGetUniformLocation(programID, name.c_str()), vec3.x, vec3.y, vec3.z);
}

void Shader::setUniform(const std::string& name, const Vector2& vec2) const {
    glUniform2f(glGetUniformLocation(programID, name.c_str()), vec2.u, vec2.v);
}

void Shader::setUniform(const std::string& name, const Matrix4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, mat.ptr());
}

void Shader::setUniformTranspose(const std::string& name, const Matrix4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_TRUE, mat.ptr());
}
void Shader::setUniform(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
}