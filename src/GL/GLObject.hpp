
#pragma once

#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace GL {

    template<typename T>
    class GLObject {
    public:

        GLObject() = default;

        void create() {
            if (id == 0)
                T::Gen(1, &id);
        }

        template<typename... Args>
        requires requires(GLuint id, Args&&... as) {
            T::Bind(id, std::forward<Args>(as)...);
        }
        void bind(Args&&... args) const {
            T::Bind(id, std::forward<Args>(args)...);
        }

        template<typename... Args>
        requires requires(GLsizeiptr s, const void* p, GLenum u) {
            T::Data(s, p, u);
        }
        void data(GLsizeiptr size, const void* pointer, GLenum usage) const {
            T::Data(size, pointer, usage);
        }

        void unbind() const requires requires { T::Unbind(); } {
            T::Unbind();
        }

        ~GLObject() { 
            if (id != 0) 
                T::Del(1, &id); 
        }

        GLObject(GLObject&& other) noexcept : id(other.id) {
            other.id = 0;
        }

        GLObject& operator=(GLObject&& other) noexcept {
            if (this != &other) {
                if (id != 0)
                    T::Del(1, &id);
                id = other.id;
                other.id = 0;
            }
            return *this;
        }

        GLuint get() const { return id; }

        template<typename VertexT>
        requires requires(GLuint index, GLint size, GLenum type, GLboolean norm, GLsizei stride, std::size_t offset) {
            T::Attrib(index, size, type, norm, stride, offset);
        }
        void setLayout() const {
            for (const auto& a : VertexT::attributes) {
                T::Attrib( a.index, a.size, a.type, GL_FALSE, sizeof(VertexT), a.offset );
            }
        }

    private:

        GLuint id = 0;

        GLObject(const GLObject&) = delete;
        GLObject& operator=(const GLObject&) = delete;

    };

    struct VertexArrayObject {
        static void Gen(GLsizei n, GLuint* ids) { glGenVertexArrays(n, ids); }
        static void Del(GLsizei n, const GLuint* ids) { glDeleteVertexArrays(n, ids); } 
        static void Bind(GLuint id) { glBindVertexArray(id); }
        static void Unbind() { glBindVertexArray(0); }
        
        static void Attrib(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, std::size_t offset) {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, size, type, normalized, stride, (void*)offset);
        }
    };

    template<GLenum target>
    struct BufferObject {
        static void Gen(GLsizei n, GLuint* ids) { glGenBuffers(n, ids); }
        static void Del(GLsizei n, const GLuint* ids) { glDeleteBuffers(n, ids); }
        static void Bind(GLuint id) { glBindBuffer(target, id); }
        
        static void Data(GLsizeiptr size, const void* data, GLenum usage) {
            glBufferData(target, size, data, usage);
        }
    };

    struct Texture {
        static void Gen(GLsizei n, GLuint* ids) { glGenTextures(n, ids); }
        static void Del(GLsizei n, const GLuint* ids) { glDeleteTextures(n, ids); }
        static void Bind(GLuint id, GLenum target = GL_TEXTURE_2D) { glBindTexture(target, id); }
    };

    using TextureObject = GLObject<Texture>;
    using VertexBuffer  = GLObject<BufferObject<GL_ARRAY_BUFFER>>;
    using ElementBuffer = GLObject<BufferObject<GL_ELEMENT_ARRAY_BUFFER>>;
    using VertexArray   = GLObject<VertexArrayObject>;
}
