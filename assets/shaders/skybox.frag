#version 330 core

in vec3 Direction;

out vec4 FragColor;

uniform sampler2D skybox;

uniform vec3 skyColor;
uniform float skyIntensity;

const float PI = 3.14159265359;

void main()
{
    vec3 dir = normalize(Direction);

    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = 0.5 - asin(clamp(dir.y, -1.0, 1.0)) / PI;

    vec4 color = vec4(skyColor * skyIntensity, 1.0);
    color *= texture(skybox, vec2(u, v));
    FragColor = color;
}