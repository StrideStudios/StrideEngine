#include "material\input_structures.hlsl"
#include "material\texturing.hlsl"

struct PSInput {
    [[vk::location(0)]] Normal : NORMAL0;
    [[vk::location(1)]] Color : COLOR0;
    [[vk::location(2)]] UV0 : TEXCOORD0;
};

struct PSOutput {
    float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output = (PSOutput)0;
    output.Color = float4(0.33f, 0.f, 0.f, 1.f);
}
