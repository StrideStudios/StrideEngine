#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "Material.h"
#include "MeshPass.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

class CVulkanRenderer;

class CGPUScene : public IDestroyable {

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

	CGPUScene(CVulkanRenderer* renderer);

	void render(CVulkanRenderer* renderer, VkCommandBuffer cmd);

	void update(CVulkanRenderer* renderer);

	FrameData m_Frames[gFrameOverlap];

	Data m_GPUSceneData;

	VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

	SBuffer m_GPUSceneDataBuffer;

	//
	// Passes
	//

	SMeshPass basePass{EMeshPass::BASE_PASS};

	//
	// Default Data
	// TODO: probably shouldnt be in gpu scene
	//

	SImage m_ErrorCheckerboardImage;

	SMaterialInstance m_DefaultData;

	VkSampler m_DefaultSamplerLinear;
	VkSampler m_DefaultSamplerNearest;
};
