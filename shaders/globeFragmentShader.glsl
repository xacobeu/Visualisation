#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D heightmap;
uniform sampler2D colormap;

void main()
{
    // === Sample textures ===
    float height = texture(heightmap, TexCoord).r;
    vec3 baseColor = texture(colormap, TexCoord).rgb;

    // === Base Color Enhancement ===
    float gray = dot(baseColor, vec3(0.299, 0.587, 0.114));
    baseColor = mix(vec3(gray), baseColor, 1.4);

    // Gamma + brightness boost
    baseColor = pow(baseColor, vec3(0.8)) * 1.2;

    // Height influence
    baseColor *= (0.6 + 0.4 * height);

    // === Soft Lighting ===
    vec3 lightDir = normalize(vec3(0.4, 0.6, 1.0));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    float ambient = 0.7;

    vec3 finalColor = baseColor * (ambient + diff);

    // Clamp the colors
    finalColor = clamp(finalColor, 0.0, 1.0);

    FragColor = vec4(finalColor, 1.0);
}