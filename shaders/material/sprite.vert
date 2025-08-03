#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_EXT_scalar_block_layout : enable

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"

//layout (location = 0) in vec2 inUV0;
//layout (location = 1) in vec2 inUV1;

layout (location = 0) out vec2 outUV;
layout (location = 1) out flat uint outTextureID;

struct Sprite {
    vec4 posScale;
    uint textureID;
    vec3 padding; // TODO: use for padding
};

layout (buffer_reference, scalar) readonly buffer SpriteBuffer {
    Sprite sprites[];
};

void main() {
    uint vertexID = 1 << (gl_VertexIndex % 6);
    vec2 texCoord = vec2((0x1Cu & vertexID) != 0, (0xEu & vertexID) != 0);

    // Construct an orthogonal projection matrix
    mat4 viewProj = mat4(
        2 / sceneData.screenSize.x, 0, 0, 0,
        0, 2 / sceneData.screenSize.y, 0, 0,
        0, 0, -2, 0,
        -1, -1, 0, 1
    );

    uint u1 = uint(PushConstants.constant[0].x);
    uint u2 = uint(PushConstants.constant[0].y);

    uint64_t unsignedKey = (uint64_t(u1) << 32) | uint64_t(u2);
    SpriteBuffer buf = SpriteBuffer(unsignedKey);

    Sprite sprite = buf.sprites[gl_InstanceIndex];

    // Use position and scale to construct a sprite matrix
    mat4 spriteMatrix = mat4(
        sprite.posScale.z, 0, 0, 0,
        0, sprite.posScale.w, 0, 0,
        0, 0, 1, 0,
        sprite.posScale.x, sprite.posScale.y, 0, 1
    );

    gl_Position = viewProj * spriteMatrix * vec4(texCoord, 0.f, 1.f);
    outUV = texCoord;
    outTextureID = sprite.textureID;
}