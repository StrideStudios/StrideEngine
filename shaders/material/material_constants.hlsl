// Push constants block
[[vk::push_constant]]
struct {
	float4 constants[8];

	float4 operator[](uint i) {
		return constants[i];
	}
} PushConstants;