#version 330 core

layout (location = 0) in vec3 inPosition;

out vec3 Direction;

uniform mat4 projection;
uniform mat4 view;

uniform mat3 skyRotation;

void main()
{
    Direction = skyRotation * inPosition;

    vec4 pos = projection * mat4(mat3(view)) * vec4(inPosition, 1.0);

    gl_Position = pos.xyww;
}