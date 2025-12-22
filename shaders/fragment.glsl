#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    // Simple lighting calculation using the normal
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 0.5, 0.2);
    vec3 ambient = 0.3 * vec3(1.0, 0.5, 0.2);

    FragColor = vec4(ambient + diffuse, 1.0);
}
