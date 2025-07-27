layout(set = 0, binding = 0) uniform  SceneData{

    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData{

    vec4 colorFactors;
    vec4 metal_rough_factors;
    vec4 samplerIDs;

} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;

layout (set = 2, binding = 0) uniform texture2D textures[];
layout (set = 2, binding = 1) uniform sampler samplers[];

vec4 sampleTexture2DNearest(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[0])), uv);
}

vec4 sampleTexture2DLinear(uint texID, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures[texID], samplers[1])), uv);
}