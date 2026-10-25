#version 330 core

in vec2 uv;

uniform sampler2D scene;
uniform float threshold;
uniform float knee;

out vec4 FragColor;

void main() {
    vec4 c = texture(scene, uv);
    float l = max(max(c.r, c.g), c.b);
    float soft = clamp((l - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    FragColor = vec4(c.rgb * max(step(threshold, l), soft), 1.0);
}