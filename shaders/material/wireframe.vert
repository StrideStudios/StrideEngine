#include "material\scene_data.hlsl"
#include "material\material_constants.hlsl"

struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(4)]] float4x4 InTransform : POSITION1;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float4 Color : COLOR0;
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID) {
    VSOutput output = (VSOutput)0;

    float4x4 constructedMatrix = float4x4(
        PushConstants[0],
        PushConstants[1],
        PushConstants[2],
        PushConstants[3]
    );

    // TODO: unfortunately, constructedMatrix is column major because of glm
    output.Position = mul(sceneData.viewproj, mul(mul(input.InTransform, float4(input.Position.xyz, 1.0)), constructedMatrix));
    // was: gl_Position = sceneData.viewproj * inTransform * constructedMatrix * position;

    /*outColor = vec4(
    float(inColor & 0xffu) / 255.f,
    float((inColor & 0xff00u) >> 8) / 255.f,
    float((inColor & 0xff0000u) >> 16) / 255.f,
    float(inColor >> 24) / 255.f
    );*/

    output.Color = PushConstants[4];

    return output;
}