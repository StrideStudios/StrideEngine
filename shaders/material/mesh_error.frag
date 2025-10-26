#include "material\scene_data.hlsl"
#include "material\material_constants.hlsl"
#include "material\texturing.hlsl"

struct PSInput {
    [[vk::location(0)]] float3 Normal : NORMAL0;
    [[vk::location(1)]] float4 Color : COLOR0;
    [[vk::location(2)]] float2 UV0 : TEXCOORD0;
};

struct PSOutput {
    float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output = (PSOutput)0;

    float3 color = sampleTexture2DNearest(0,input.UV0).xyz;

    output.Color = float4(color, 1.f);

    return output;
}
