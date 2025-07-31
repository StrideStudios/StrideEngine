#version 460

#extension GL_EXT_nonuniform_qualifier : enable

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"
#include "material\texturing.glsl"

layout (location = 0) in vec3 vertexNormal;
layout (location = 1) in vec4 vertexColor;
layout (location = 2) in vec2 uv0;

layout (location = 0) out vec4 outColor;

void main() {
    float lightValue = max(dot(vertexNormal, sceneData.sunlightDirection.xyz), 0.1f);

    vec3 color = vertexColor.xyz * sampleTexture2DLinear(uint(PushConstants.constant[0].x),uv0).xyz;
    vec3 ambient = color *  sceneData.ambientColor.xyz;

    outColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient, 1.f);
}
