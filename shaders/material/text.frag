#include "material\material_constants.hlsl"
#include "material\texturing.hlsl"

struct PSInput {
    [[vk::location(0)]] float2 UV0 : TEXCOORD0;
};

struct PSOutput {
    float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output = (PSOutput)0;

    float4 color = sampleTexture2DNearest(uint(PushConstants[0].x), input.UV0);

    // Masked Alpha
    //if (color.r < 0.1) discard;

    output.Color = float4(1.0, 1.0, 1.0, color.r);

    return output;
}