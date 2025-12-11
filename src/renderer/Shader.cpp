#include "Shader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    compile(vertexSrc, fragmentSrc);
}

bool Shader::compile(const std::string& vsSrc, const std::string& fsSrc) {
    GLuint vs = compileSingle(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileSingle(GL_FRAGMENT_SHADER, fsSrc);

    if (!vs || !fs) return false;

    bool ok = link(vs, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return ok;
}

GLuint Shader::compileSingle(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::compileFromFiles(const std::string& vertPath, const std::string& fragPath) {
    std::string vs = loadTextFile(vertPath);
    std::string fs = loadTextFile(fragPath);
    return compile(vs, fs);
}

bool Shader::link(GLuint vs, GLuint fs) {
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Shader link error:\n" << log << "\n";
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    return true;
}

void Shader::bind() const {
    glUseProgram(program);
}

void Shader::unbind() {
    glUseProgram(0);
}

GLint Shader::uniform(const std::string& name) {
    auto it = uniformCache.find(name);
    if (it != uniformCache.end())
        return it->second;

    GLint loc = glGetUniformLocation(program, name.c_str());
    uniformCache[name] = loc;
    return loc;
}

void Shader::setMat4(const std::string& name, const float* ptr) {
    glUniformMatrix4fv(uniform(name), 1, GL_FALSE, ptr);
}

void Shader::setInt(const std::string& name, int v) {
    glUniform1i(uniform(name), v);
}

void Shader::setFloat(const std::string& name, float v) {
    glUniform1f(uniform(name), v);
}

void Shader::setVec2(const std::string& name, float x, float y) {
    glUniform2f(uniform(name), x, y);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(uniform(name), x, y, z);
}

std::string Shader::loadTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << "\n";
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}