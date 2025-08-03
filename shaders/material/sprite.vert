#version 460

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"

//layout (location = 0) in vec2 inUV0;
//layout (location = 1) in vec2 inUV1;

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

    vec4 posScale = PushConstants.constant[0];

    // Use position and scale to construct a sprite matrix
    mat4 spriteMatrix = mat4(
        posScale.z, 0, 0, 0,
        0, posScale.w, 0, 0,
        0, 0, 1, 0,
        posScale.x, posScale.y, 0, 1
    );

    gl_Position = viewProj * spriteMatrix * vec4(texCoord, 0.f, 1.f);
    outUV = texCoord;
}