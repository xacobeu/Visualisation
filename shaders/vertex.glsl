#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uView;
uniform mat4 uProj;

void main() {
    TexCoord = aUV;
    Normal = aNormal;

    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
