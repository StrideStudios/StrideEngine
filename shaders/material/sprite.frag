#version 460

#extension GL_EXT_nonuniform_qualifier : enable

#include "material\material_constants.glsl"
#include "material\texturing.glsl"

layout (location = 0) in vec2 uv0;

layout (location = 0) out vec4 outColor;

void main() {
    vec4 color = sampleTexture2DNearest(uint(PushConstants[0].x), uv0);

    // Masked Alpha
    if (color.a < 0.1) discard;

    outColor = color;
}