#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "material\input_structures.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in float inUVx;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inUVy;
layout(location = 4) in vec4 inColor;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec2 outUV;

void main() {
    vec4 position = vec4(inPos, 1.f);

    gl_Position =  sceneData.viewproj * position;

    outNormal = inNormal;
    outColor = inColor;
    outUV.x = inUVx;
    outUV.y = inUVy;
}
