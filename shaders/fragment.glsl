#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
