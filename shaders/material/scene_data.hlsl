struct SceneData {
    float4x4 viewproj;
    float2 screenSize;
    float2 invScreenSize;
    float4 ambientColor;
    float4 sunlightDirection; //w for sun power
    float4 sunlightColor;
};

ConstantBuffer<SceneData> sceneData : register(b2);