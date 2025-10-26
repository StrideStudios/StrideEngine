#include "material\scene_data.hlsl"
#include "material\material_constants.hlsl"

//layout (location = 0) in vec2 inUV0;
//layout (location = 1) in vec2 inUV1;

struct VSInput {
    [[vk::location(0)]] float4x4 Transform : POSITION0;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float2 UV0 : TEXCOORD0;
};

static const float2 spriteCoords[6] = {
    float2(0,0),
    float2(0,1),
    float2(1,1),
    float2(1,1),
    float2(1,0),
    float2(0,0)
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID) {
    VSOutput output = (VSOutput)0;

    float2 texCoord = spriteCoords[VertexIndex % 6u];

    // Construct an orthogonal projection matrix
    float4x4 viewProj = float4x4(
        2.0 / sceneData.screenSize.x, 0.0, 0.0, 0.0,
        0.0, 2.0 / sceneData.screenSize.y, 0.0, 0.0,
        0.0, 0.0, -2.0, 0.0,
        -1.0, -1.0, 0.0, 1.0
    );

    // TODO: unfortunately, input.Transform is column major because of glm
    // Its also possible that texCoord isn't multiplied properly in standard format
    output.Position = mul(mul(float4(texCoord, 0.0, 1.0), input.Transform), viewProj);
    // was: gl_Position = viewProj * inTransform * vec4(texCoord, 0.f, 1.f);

    output.UV0 = texCoord;

    return output;
}