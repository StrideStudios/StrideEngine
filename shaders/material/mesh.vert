#version 460

#include "material\scene_data.glsl"
#include "material\material_constants.glsl"

layout(location = 0) in uvec4 inPosNormUV;
layout(location = 1) in uint inColor;

layout(location = 2) in mat4 inTransform;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec2 outUV;

// Seems complicated and slow, but bit operations are very quick
float convertUintHalvesToFloat(uint inValue) {
    const uint e = (inValue & 0x7C00u) >> 10u;
    const uint m = (inValue & 0x03FFu) << 13u;
    const uint v = floatBitsToUint(float(m)) >> 23u;
    return uintBitsToFloat((inValue & 0x8000u) << 16u | (e != 0u ? 1 : 0) * ((e + 112u) << 23u | m) | ((e == 0u ? 1 : 0) & (m != 0u ? 1 : 0)) * ((v - 37u) << 23u|((m << (150u - v)) & 0x007FE000u)));
}

void main() {
    vec4 position = vec4(
    convertUintHalvesToFloat(inPosNormUV.x & 0xffffu),
    convertUintHalvesToFloat(inPosNormUV.x >> 16u),
    convertUintHalvesToFloat(inPosNormUV.y >> 16u)
    , 1.f
    );

    outNormal = vec3(
    convertUintHalvesToFloat(inPosNormUV.y >> 16u),
    convertUintHalvesToFloat(inPosNormUV.z & 0xffffu),
    convertUintHalvesToFloat(inPosNormUV.z & 0xffffu)
    );

    gl_Position = sceneData.viewproj * inTransform * position;

    outColor = vec4(
    float(inColor & 0xffu) / 255.f,
    float((inColor & 0xff00u) >> 8) / 255.f,
    float((inColor & 0xff0000u) >> 16) / 255.f,
    float(inColor >> 24) / 255.f
    );

    // UVs are halves, packed into a single int
    outUV.x = convertUintHalvesToFloat(inPosNormUV.w & 0xffffu);
    outUV.y = convertUintHalvesToFloat(inPosNormUV.w >> 16u);
}
