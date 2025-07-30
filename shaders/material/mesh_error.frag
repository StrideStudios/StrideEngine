#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "material\input_structures.glsl"
#include "material\texturing.glsl"

layout (location = 0) in vec3 vertexNormal;
layout (location = 1) in vec3 vertexColor;
layout (location = 2) in vec2 uv0;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 color = sampleTexture2DNearest(0,uv0).xyz;

    outColor = vec4(color, 1.f);
}
