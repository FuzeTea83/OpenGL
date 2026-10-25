#version 330 core

in vec2 uv;

uniform sampler2D image;
uniform vec2 direction;

out vec4 FragColor;

void main() {
    vec3 c = texture(image, uv).rgb * 0.227027;
    for (int i = 1;i < 5; i++) {
        float w = i == 1 ? 0.1945946 : i == 2 ? 0.1216216 : i == 3 ? 0.054054 : i == 4 ? 0.0162162 : 0.0;c += texture(image, uv + direction * float(i)).rgb * w;c += texture(image, uv - direction * float(i)).rgb * w;
    }
    FragColor = vec4(c, 1.0);
}