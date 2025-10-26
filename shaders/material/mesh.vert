#include "material\scene_data.hlsl"
#include "material\material_constants.hlsl"

struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(1)]] uint UV0 : TEXCOORD0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
    [[vk::location(3)]] uint Color : COLOR0;
    [[vk::location(4)]] float4x4 Transform : POSITION1;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float3 Normal : NORMAL0;
    [[vk::location(1)]] float4 Color : COLOR0;
    [[vk::location(2)]] float2 UV0 : TEXCOORD0;
};

// Seems complicated and slow, but bit operations are very quick
float convertUintHalvesToFloat(uint inValue) {
    const uint e = (inValue & 0x7C00u) >> 10u;
    const uint m = (inValue & 0x03FFu) << 13u;
    const uint v = asuint(float(m)) >> 23u;
    return asfloat((inValue & 0x8000u) << 16u | (e != 0u ? 1 : 0) * ((e + 112u) << 23u | m) | ((e == 0u ? 1 : 0) & (m != 0u ? 1 : 0)) * ((v - 37u) << 23u|((m << (150u - v)) & 0x007FE000u)));
}

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID) {
    VSOutput output = (VSOutput)0;

    output.Normal = input.Normal;

    output.Position = mul(sceneData.viewproj, mul(input.Transform, float4(input.Position.xyz, 1.0)));

    output.Color = float4(
        float(input.Color & 0xffu) / 255.f,
        float((input.Color & 0xff00u) >> 8) / 255.f,
        float((input.Color & 0xff0000u) >> 16) / 255.f,
        float(input.Color >> 24) / 255.f
    );

    // UVs are halves, packed into a single int
    output.UV0.x = convertUintHalvesToFloat(input.UV0 & 0xffffu);
    output.UV0.y = convertUintHalvesToFloat(input.UV0 >> 16u);

    return output;
}
