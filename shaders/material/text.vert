#version 460

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"

layout (location = 0) in vec2 inUV0;
layout (location = 1) in vec2 inUV1;

layout(location = 2) in mat4 inTransform;

layout (location = 0) out vec2 outUV;

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

    /*uint u1 = uint(PushConstants[0].x);
    uint u2 = uint(PushConstants[0].y);

    uint64_t unsignedKey = (uint64_t(u1) << 32) | uint64_t(u2);
    SpriteBuffer buf = SpriteBuffer(unsignedKey);

    Sprite sprite = buf.sprites[gl_InstanceIndex];*/ //TODO: buffer address can be made from uint64

    gl_Position = viewProj * inTransform * vec4(texCoord, 0.f, 1.f);
    outUV = (1.f - texCoord) * inUV0 + texCoord * inUV1;
}