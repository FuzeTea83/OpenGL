#version 330 core

in vec2 uv;

out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform float exposure;


void main() {
    vec3 hdr = texture(scene, uv).rgb + texture(bloom, uv).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);
    FragColor = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}