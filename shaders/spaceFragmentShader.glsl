#version 330 core

in vec2 uv;
out vec4 FragColor;

uniform vec2 uResolution;

// Tiny random function
uint lowbias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

void main() {

    vec2 pos = uv * 700.0;
    
    float h = lowbias32(uint(floor(pos.x)) + uint(floor(pos.y) * 65536U)) / float(0xffffffffU);

    FragColor = vec4(vec3(step(0.995, h)), 1.0);
}