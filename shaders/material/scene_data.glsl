layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewproj;
    vec2 screenSize;
    vec2 invScreenSize;
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;