#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "Material.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

class CVulkanRenderer;

struct SRenderObject {
	uint32 indexCount;
	uint32 firstIndex;
	VkBuffer indexBuffer;

	SMaterialInstance* material;

	Matrix4f transform;
	VkDeviceAddress vertexBufferAddress;
};

struct SRenderContext {
	std::vector<SRenderObject> opaqueSurfaces;
};

// base class for a renderable dynamic object
class IRenderable {

	virtual void render(const Matrix4f& topMatrix, SRenderContext& ctx) = 0;

};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct SNode : IRenderable {

	// parent pointer must be a weak pointer to avoid circular dependencies
	std::weak_ptr<SNode> parent;
	std::vector<std::shared_ptr<SNode>> children;

	Matrix4f localTransform;
	Matrix4f worldTransform;

	void refreshTransform(const Matrix4f& parentMatrix)
	{
		worldTransform = parentMatrix * localTransform;
		for (auto c : children) {
			c->refreshTransform(worldTransform);
		}
	}

	void render(const Matrix4f& topMatrix, SRenderContext& ctx) override
	{
		// draw children
		for (auto& c : children) {
			c->render(topMatrix, ctx);
		}
	}
};

struct SMeshNode : public SNode {

	std::shared_ptr<SStaticMesh> mesh;

	void render(const Matrix4f& topMatrix, SRenderContext& ctx) override;
};


struct SGLTFMetallic_Roughness {
	SMaterialPipeline opaquePipeline;
	SMaterialPipeline transparentPipeline;

	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {
		Vector4f colorFactors;
		Vector4f metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		Vector4f extra[14];
	};

	struct MaterialResources {
		SImage colorImage;
		VkSampler colorSampler;
		SImage metalRoughImage;
		VkSampler metalRoughSampler;
		VkBuffer dataBuffer;
		uint32 dataBufferOffset;
	};

	SDescriptorWriter writer;

	void buildPipelines(CVulkanRenderer* renderer, class CGPUScene* gpuScene);

	SMaterialInstance writeMaterial(EMaterialPass pass, const MaterialResources& resources, SDescriptorAllocator& descriptorAllocator);
};

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

	SRenderContext m_MainRenderContext;

	std::unordered_map<std::string, std::shared_ptr<SNode>> m_LoadedNodes;

	Data m_GPUSceneData;

	VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

	SBuffer m_GPUSceneDataBuffer;

	//
	// Default Data
	// TODO: probably shouldnt be in gpu scene
	//

	SImage m_WhiteImage;
	SImage m_BlackImage;
	SImage m_GreyImage;
	SImage m_ErrorCheckerboardImage;

	SGLTFMetallic_Roughness metalRoughMaterial;

	SMaterialInstance m_DefaultData;

	VkSampler m_DefaultSamplerLinear;
	VkSampler m_DefaultSamplerNearest;
};
