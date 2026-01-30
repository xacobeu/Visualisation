#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor; // New attribute

out vec4 vColor;
uniform mat4 u_Transform;

void main() {
    gl_Position = u_Transform * vec4(aPos, 1.0);
    vColor = aColor; // Pass to fragment
}
