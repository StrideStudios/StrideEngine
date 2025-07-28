layout (set = 2, binding = 0) uniform texture2D textures[];
layout (set = 2, binding = 1) uniform sampler samplers[];

//push constants block
layout( push_constant ) uniform constants
{
    layout(offset = 72) uint colorId; //Useful for color, normal, metallic/specular/roughness, and one other texture
} PushConstants;


vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[0])), uv);
}

vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[1])), uv);
}