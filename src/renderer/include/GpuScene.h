#pragma once

#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "Material.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

class CVulkanRenderer;

class CGPUScene : public IDestroyable, public IInitializable<> {

	struct Data {
		Matrix4f view;
		Matrix4f proj;
		Matrix4f viewProj;
		Vector4f ambientColor;
		Vector4f sunlightDirection; // w for sun power
		Vector4f sunlightColor;
	};

	struct FrameData {
		VkDescriptorSet sceneDescriptor;
	};

public:

	CGPUScene() = default;

	void init() override;

	void render(VkCommandBuffer cmd);

	void update();

	FrameData m_Frames[gFrameOverlap];

	Data m_GPUSceneData;

	VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

	SBuffer m_GPUSceneDataBuffer;

	//
	// Passes
	//

	class CMeshPass* basePass;

	//
	// Default Data
	// TODO: probably shouldnt be in gpu scene
	//

	SImage m_ErrorCheckerboardImage;

	std::shared_ptr<CMaterial> mErrorMaterial;

	VkSampler m_DefaultSamplerLinear;
	VkSampler m_DefaultSamplerNearest;
};
