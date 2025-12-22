#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 u_Transform;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    TexCoord = aUV;

    gl_Position = u_Transform * vec4(aPos, 1.0);
}
