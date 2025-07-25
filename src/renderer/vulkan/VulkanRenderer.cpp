#include "VulkanRenderer.h"

#include <cmath>
#include <iostream>

#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"
#include "VkBootstrap.h"

#include "Engine.h"
#include "ShaderCompiler.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "EngineBuffers.h"
#include "ResourceManager.h"

#define COMMAND_CATEGORY "Engine"
ADD_COMMAND(bool, UseVsync, true);
#undef COMMAND_CATEGORY

CVulkanRenderer::CVulkanRenderer(): m_VSync(UseVsync.get()) {}

CVulkanRenderer::~CVulkanRenderer() = default;

void CVulkanRenderer::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
	VkDevice device = CEngine::device();

	const CVulkanRenderer& renderer = CEngine::renderer();

	VkCommandBuffer cmd = renderer.m_UploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = CVulkanInfo::submitInfo(&cmd);

	vkResetFences(device, 1, &renderer.m_UploadContext._uploadFence);

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(CEngine::renderer().m_GraphicsQueue, 1, &submit, renderer.m_UploadContext._uploadFence));

	vkWaitForFences(device, 1, &renderer.m_UploadContext._uploadFence, true, 9999999999);

	// reset the command buffers inside the command pool
	vkResetCommandPool(device, renderer.m_UploadContext._commandPool, 0);
}

void CVulkanRenderer::init() {
	// Ensure the renderer is only created once
	astsOnce(CVulkanRenderer);

	// Use vkb to get a Graphics queue
	m_GraphicsQueue = CEngine::device().get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = CEngine::device().get_queue_index(vkb::QueueType::graphics).value();

	VkCommandPoolCreateInfo uploadCommandPoolInfo = CVulkanInfo::createCommandPoolInfo(CEngine::renderer().m_GraphicsQueueFamily);
	//create pool for upload context
	m_UploadContext._commandPool = mGlobalResourceManager.allocateCommandPool(uploadCommandPoolInfo);

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(m_UploadContext._commandPool, 1);
	m_UploadContext._commandBuffer = CResourceManager::allocateCommandBuffer(cmdAllocInfo);

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	m_UploadContext._uploadFence = mGlobalResourceManager.allocateFence(fenceCreateInfo);

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: m_Frames) {
			frame.mCommandPool = mGlobalResourceManager.allocateCommandPool(commandPoolInfo);

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(frame.mCommandPool, 1);

			frame.mMainCommandBuffer = CResourceManager::allocateCommandBuffer(cmdAllocInfo);

			std::vector<SDescriptorAllocator::PoolSizeRatio> frameSizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
			};

			// TODO: DescriptorAllocator as part of ResourceManager
			frame.mDescriptorAllocator = SDescriptorAllocator();
			frame.mDescriptorAllocator.init(1000, frameSizes);

			mGlobalResourceManager.pushF([&] {
				frame.mDescriptorAllocator.destroy();
			});
		}
	}

	m_EngineTextures = std::make_unique<CEngineTextures>();

	m_EngineBuffers = std::make_unique<CEngineBuffers>(this);

	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<SDescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_GlobalDescriptorAllocator.init(10, sizes);

	//make the descriptor set layout for our compute draw
	{
		SDescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_DrawImageDescriptorLayout = builder.build(VK_SHADER_STAGE_COMPUTE_BIT);
	}

	initDescriptors();

	initImGui();
}

void CVulkanRenderer::destroy() {

	CEngineSettings::destroy();

	m_GlobalDescriptorAllocator.destroy();

	mGlobalResourceManager.flush();

	vkDestroyDescriptorSetLayout(CEngine::device(), m_GPUSceneDataDescriptorLayout, nullptr);

	vkDestroyDescriptorSetLayout(CEngine::device(), m_DrawImageDescriptorLayout, nullptr);

	m_EngineTextures->destroy();
}

void CVulkanRenderer::draw() {

	if (m_VSync != UseVsync.get()) {
		m_VSync = UseVsync.get();
		msgs("Reallocating Swapchain to {}", UseVsync.get() ? "enable VSync." : "disable VSync.");
		m_EngineTextures->getSwapchain().recreate(UseVsync.get());
	}

	// Make sure that the swapchain is not dirty before recreating it
	while (m_EngineTextures->getSwapchain().isDirty()) {
		// Wait for gpu before recreating swapchain
		waitForGpu();

		m_EngineTextures->reallocate();

		updateDescriptors();
	}

	// Wait for the previous render to stop
	m_EngineTextures->getSwapchain().wait(getFrameIndex());

	// Get command buffer from current frame
	VkCommandBuffer cmd = getCurrentFrame().mMainCommandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	// Tell Vulkan the buffer will only be used once
	VkCommandBufferBeginInfo cmdBeginInfo = CVulkanInfo::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Get the current swapchain image
	const auto [swapchainImage, swapchainImageView, swapchainImageIndex] = m_EngineTextures->getSwapchain().getSwapchainImage(getFrameIndex());

	// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
	m_EngineTextures->getSwapchain().reset(getFrameIndex());

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Reset descriptor allocators
	getCurrentFrame().mDescriptorAllocator.clear();

	//allocate a new uniform buffer for the scene data

	// GPU Scene is only needed in render TODO: (should put in frame)
	m_GPUSceneDataBuffer = getCurrentFrame().mFrameResourceManager.allocateBuffer(sizeof(SGPUSceneData), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	//write the buffer
	auto* sceneUniformData = (SGPUSceneData*)m_GPUSceneDataBuffer->GetMappedData();
	m_SceneData = *sceneUniformData;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = getCurrentFrame().mDescriptorAllocator.allocate(m_GPUSceneDataDescriptorLayout);

	SDescriptorWriter writer;
	writer.writeBuffer(0, m_GPUSceneDataBuffer->buffer, sizeof(SGPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.updateSet(globalDescriptor);

	render(cmd);

	// Flush temporary frame data after it is no longer needed
	getCurrentFrame().mFrameResourceManager.flush();

	// Make the swapchain image into presentable mode
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	auto [width, height, depth] = m_EngineTextures->mDrawImage->mImageExtent;
	CVulkanUtils::copyImageToImage(cmd, m_EngineTextures->mDrawImage->mImage, swapchainImage, {width, height}, m_EngineTextures->getSwapchain().mSwapchain->extent);

	// Set swapchain layout so it can be used by ImGui
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Render Engine settings
	CEngineSettings::render(cmd, m_EngineTextures->getSwapchain().mSwapchain->extent, swapchainImageView);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	m_EngineTextures->getSwapchain().submit(cmd, m_GraphicsQueue, getFrameIndex(), swapchainImageIndex);

	//increase the number of frames drawn
	m_FrameNumber++;
}

void CVulkanRenderer::waitForGpu() const {
	// Make sure the gpu is not working
	vkDeviceWaitIdle(CEngine::device());

	m_EngineTextures->getSwapchain().wait(getFrameIndex());
}

void CVulkanRenderer::initDescriptors() {

	//allocate a descriptor set for our draw image
	m_DrawImageDescriptors = m_GlobalDescriptorAllocator.allocate(m_DrawImageDescriptorLayout);

	{
		SDescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		m_GPUSceneDataDescriptorLayout = builder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	updateDescriptors();
}

void CVulkanRenderer::updateDescriptors() {
	SDescriptorWriter writer;

	writer.writeImage(0, m_EngineTextures->mDrawImage->mImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

	writer.updateSet(m_DrawImageDescriptors);
}

void CVulkanRenderer::initImGui() {
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo poolCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = (uint32_t)std::size(poolSizes),
		.pPoolSizes = poolSizes
	};

	m_ImGuiDescriptorPool = mGlobalResourceManager.allocateDescriptorPool(poolCreateInfo);

	// Setup Dear ImGui context
	CEngineSettings::init(m_GraphicsQueue, m_ImGuiDescriptorPool, m_EngineTextures->getSwapchain().mFormat);
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}
