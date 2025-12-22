#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>

namespace GL {
    
    class Shader {
    public:

        Shader() = default;
        Shader(const std::string& vertexSrc, const std::string& fragmentSrc);

        static std::string loadTextFile(const std::string& path);
        bool compileFromFiles(const std::string& vertPath, const std::string& fragPath);
        bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);
        void bind() const;
        static void unbind();

        GLint uniform(const std::string& name);

        void setMat4(const std::string& name, const float* ptr);
        void setInt(const std::string& name, int v);
        void setFloat(const std::string& name, float v);
        void setVec2(const std::string& name, float x, float y);
        void setVec3(const std::string& name, float x, float y, float z);

        GLuint id() const { return program; }


    private:

        GLuint program = 0;
        std::unordered_map<std::string, GLint> uniformCache;

        GLuint compileSingle(GLenum type, const std::string& src);
        bool link(GLuint vs, GLuint fs);
    };

} // namespace GL
