#pragma once

#include <memory>

#include "EngineTextures.h"
#include "EngineBuffers.h"
#include "VulkanUtils.h"

class CEngineTextures;
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

class CVulkanRenderer {

	friend class CEngineTextures;

	friend class CEngineBuffers;

public:

	struct SFrameData {
		VkCommandPool mCommandPool = nullptr;
		VkCommandBuffer mMainCommandBuffer = nullptr;

		CResourceDeallocator mResourceDeallocator;
	};

	CVulkanRenderer();

	virtual ~CVulkanRenderer() = default;

	void destroy();

	force_inline uint32 getFrameNumber() const { return m_FrameNumber; }

	force_inline uint32 getFrameIndex() const { return m_FrameNumber % gFrameOverlap; }

	force_inline const SFrameData& getCurrentFrame() const { return m_Frames[getFrameIndex()]; }

	no_discard CEngineTextures& getEngineTextures() { return *m_EngineTextures; }

	no_discard const CEngineTextures& getEngineTextures() const { return *m_EngineTextures; }

	// Draw to the screen
	void draw();

	// Tell children to render
	virtual void render(VkCommandBuffer cmd) = 0;

	void renderImGui(VkCommandBuffer cmd, VkImageView inTargetImageView);

	void waitForGpu() const;

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

	uint64 m_FrameNumber = 0;

	SFrameData m_Frames[gFrameOverlap];

	VkQueue m_GraphicsQueue;
	uint32 m_GraphicsQueueFamily;

	std::unique_ptr<CEngineTextures> m_EngineTextures = nullptr;

	std::unique_ptr<CEngineBuffers> m_EngineBuffers = nullptr;

	//
	// Descriptor Resources
	//

	CResourceDeallocator m_ResourceDeallocator;

	CResourceDeallocator m_DescriptorResourceDeallocator;

	SDescriptorAllocator m_GlobalDescriptorAllocator;

	VkDescriptorSet m_DrawImageDescriptors;
	VkDescriptorSetLayout m_DrawImageDescriptorLayout;

	VkDescriptorPool m_ImGuiDescriptorPool;

	//
	// Temp shader stuff
	//

	friend class CEngine;//lazy remove

};

class CNullRenderer final : public CVulkanRenderer {

public:

	void render(VkCommandBuffer cmd) override;
};