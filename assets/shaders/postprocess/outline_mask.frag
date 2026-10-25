#version 330 core
out vec4 FragColor;

in vec2 uv;

uniform sampler2D maskTexture;  // la texture de masque (pass 1)
uniform vec2 pixelSize;         // vec2(1.0/width, 1.0/height)
uniform vec4 outlineColor;      // couleur du contour
uniform float outlineWidth;     // rayon de recherche en pixels

void main()
{
    if (outlineWidth <= 0.0) {
        discard;
    }

    bool isInside = false;
    float dist = 1e6;

    for (int y = -int(outlineWidth); y <= int(outlineWidth); ++y) {
        for (int x = -int(outlineWidth); x <= int(outlineWidth); ++x) {
            vec2 offset = vec2(float(x), float(y)) * pixelSize;
            float mask = texture(maskTexture, uv + offset).r;

            if (x == 0 && y == 0)
            isInside = (mask > 0.5);

            if (mask > 0.5)
            dist = min(dist, length(vec2(float(x), float(y))));
        }
    }

    float alpha;
    if (isInside) {
        alpha = 0.0;
    } else {
        float solid = 0.3 * outlineWidth;
        float fuzzy = max(outlineWidth - solid, 0.0001);
        alpha = 1.0 - min(1.0, max(0.0, dist - solid) / fuzzy);
    }

    FragColor = outlineColor * alpha;
}