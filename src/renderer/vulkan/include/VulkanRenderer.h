#pragma once

#include <functional>
#include <memory>

#include "ResourceManager.h"
#include "VulkanUtils.h"

class CEngineTextures;
class CEngineBuffers;
class CVulkanDevice;

struct SComputePushConstants {
	Vector4f data1;
	Vector4f data2;
	Vector4f data3;
	Vector4f data4;
};

struct SComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	SComputePushConstants data;
};

struct SUploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

class CVulkanRenderer {

public:

	struct SFrameData {
		VkCommandPool mCommandPool = nullptr;
		VkCommandBuffer mMainCommandBuffer = nullptr;

		/*
		 * A resource allocator that is persistent for a single frame
		 * Good for data that only needs to exist for a single frame
		 */
		CResourceManager mFrameResourceManager;

		SDescriptorAllocator mDescriptorAllocator;
	};

	struct GPUSceneData {
		Matrix4f view;
		Matrix4f proj;
		Matrix4f viewProj;
		Vector4f ambientColor;
		Vector4f sunlightDirection; // w for sun power
		Vector4f sunlightColor;
	};

	CVulkanRenderer();

	virtual ~CVulkanRenderer();

	static void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	virtual void init();

	virtual void destroy();

	force_inline uint32 getFrameNumber() const { return m_FrameNumber; }

	force_inline uint32 getFrameIndex() const { return m_FrameNumber % gFrameOverlap; }

	force_inline const SFrameData& getCurrentFrame() const { return m_Frames[getFrameIndex()]; }

	no_discard CEngineTextures& getEngineTextures() { return *m_EngineTextures; }

	no_discard const CEngineTextures& getEngineTextures() const { return *m_EngineTextures; }

	no_discard CEngineBuffers& getEngineBuffers() { return *m_EngineBuffers; }

	no_discard const CEngineBuffers& getEngineBuffers() const { return *m_EngineBuffers; }

	// Draw to the screen
	void draw();

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) = 0;

	void renderImGui(VkCommandBuffer cmd, VkImageView inTargetImageView);

	void waitForGpu() const;

	/*
	 * This resource allocator is flushed when the renderer is destroyed
	 * This is useful for any objects that need a persistent lifetime
	 */
	CResourceManager mGlobalResourceManager;

protected:

	void initDescriptors();

	void updateDescriptors();

	void initImGui();

	//
	// Private non-const getters
	//

	force_inline SFrameData& getCurrentFrame() { return m_Frames[getFrameIndex()]; }

	//
	// Rendering Utils
	//

	bool m_VSync;

	SBuffer m_GPUSceneDataBuffer;

	uint64 m_FrameNumber = 0;

	SFrameData m_Frames[gFrameOverlap];

	SUploadContext m_UploadContext;

	VkQueue m_GraphicsQueue;
	uint32 m_GraphicsQueueFamily;

	std::unique_ptr<CEngineTextures> m_EngineTextures = nullptr;

	std::unique_ptr<CEngineBuffers> m_EngineBuffers = nullptr;

	//
	// Descriptor Resources
	//

	SDescriptorAllocator m_GlobalDescriptorAllocator;

	VkDescriptorSet m_DrawImageDescriptors;
	VkDescriptorSetLayout m_DrawImageDescriptorLayout;

	VkDescriptorPool m_ImGuiDescriptorPool;

	//
	// Scene Data
	//

	GPUSceneData m_SceneData;

	VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

	//
	// Temp shader stuff
	//

	friend class CEngine;//lazy remove

};

class CNullRenderer final : public CVulkanRenderer {

public:

	void render(VkCommandBuffer cmd) override;
};