#version 460

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in uint inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inColor;

layout(location = 4) in mat4 inTransform;

layout (location = 0) out vec4 outColor;

void main() {
    vec4 position = vec4(inPos, 1.f);

    mat4 constructedMatrix = mat4(
        PushConstants[0],
        PushConstants[1],
        PushConstants[2],
        PushConstants[3]
    );

    gl_Position = sceneData.viewproj * inTransform * constructedMatrix * position;

    /*outColor = vec4(
    float(inColor & 0xffu) / 255.f,
    float((inColor & 0xff00u) >> 8) / 255.f,
    float((inColor & 0xff0000u) >> 16) / 255.f,
    float(inColor >> 24) / 255.f
    );*/

    outColor = PushConstants[4];
}
