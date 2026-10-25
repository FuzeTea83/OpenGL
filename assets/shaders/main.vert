#version 330 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec4 inWeights;
layout (location = 4) in uvec4 inBoneIds;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform bool hasSkinning;

#define MAX_BONES 512

layout (std140) uniform BoneBlock
{
    mat4 finalBoneMatrices[MAX_BONES];
};

void main()
{
    vec4 pos = vec4(inPosition, 1.0);
    vec3 normal = inNormal;

    if (hasSkinning)
    {
        mat4 skin =
        inWeights.x * finalBoneMatrices[inBoneIds.x] +
        inWeights.y * finalBoneMatrices[inBoneIds.y] +
        inWeights.z * finalBoneMatrices[inBoneIds.z] +
        inWeights.w * finalBoneMatrices[inBoneIds.w];

        pos = skin * pos;
        normal = mat3(skin) * normal;
    }

    vec4 world = model * pos;
    FragPos = world.xyz;
    Normal = normalize(mat3(transpose(inverse(model))) * normal);
    TexCoord = inTexCoord;
    gl_Position = projection * view * world;
}