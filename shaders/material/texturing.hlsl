Texture2D textures[] : register(t0);
SamplerState samplers[] : register(s1);

float4 sampleTexture2DNearest(uint texID, float2 uv) {
    return textures[NonUniformResourceIndex(texID)].Sample(samplers[0], uv);
}

float4 sampleTexture2DLinear(uint texID, float2 uv) {
    return textures[NonUniformResourceIndex(texID)].Sample(samplers[1], uv);
}