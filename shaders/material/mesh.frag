#include "material\scene_data.hlsl"
#include "material\material_constants.hlsl"
#include "material\texturing.hlsl"

struct PSInput {
    [[vk::location(0)]] float3 VertexNormal : NORMAL0;
    [[vk::location(1)]] float4 VertexColor : COLOR0;
    [[vk::location(2)]] float2 UV0 : TEXCOORD0;
};

struct PSOutput {
    float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output = (PSOutput)0;

    float lightValue = max(dot(input.VertexNormal, sceneData.sunlightDirection.xyz), 0.1f);

    float3 color = input.VertexColor.xyz * sampleTexture2DLinear(uint(PushConstants[0].x),input.UV0).xyz;
    float3 ambient = color *  sceneData.ambientColor.xyz;

    output.Color = float4(color * lightValue * sceneData.sunlightColor.w + ambient, 1.f);

    return output;
}
