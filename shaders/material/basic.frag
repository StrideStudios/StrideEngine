struct PSInput {
    [[vk::location(0)]] float4 Color : COLOR0;
};

struct PSOutput {
    float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
    PSOutput output = (PSOutput)0;
    output.Color = input.Color;
    return output;
}