#include "VulkanRenderer.h"

#include <cmath>
#include <iostream>

#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "vulkan/vk_enum_string_helper.h"

#include "Engine.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "ShaderCompiler.h"
#include "EngineSettings.h"
#include "ResourceAllocator.h"

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
	VK_CHECK(vkCreateCommandPool(CEngine::device(), &uploadCommandPoolInfo, nullptr, &m_UploadContext._commandPool));

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(m_UploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &cmdAllocInfo, &m_UploadContext._commandBuffer));

	VkFenceCreateInfo fenceCreateInfo = CVulkanInfo::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VK_CHECK(vkCreateFence(CEngine::device(), &fenceCreateInfo, nullptr, &m_UploadContext._uploadFence));

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: m_Frames) {
			VK_CHECK(vkCreateCommandPool(CEngine::device(), &commandPoolInfo, nullptr, &frame.mCommandPool));

			m_ResourceDeallocator.push(frame.mCommandPool);

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(frame.mCommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(CEngine::device(), &cmdAllocInfo, &frame.mMainCommandBuffer));

			std::vector<SDescriptorAllocator::PoolSizeRatio> frameSizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
			};

			frame.mDescriptorAllocator = SDescriptorAllocator();
			frame.mDescriptorAllocator.init(1000, frameSizes);

			m_ResourceDeallocator.push([&] {
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

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	m_GlobalDescriptorAllocator.destroy();

	mGlobalResourceAllocator.flush();

	m_ResourceDeallocator.flush();

	for (auto & mFrame : m_Frames) {
		//mFrame.mResourceDeallocator.flush();
	}

	vkDestroyDescriptorSetLayout(CEngine::device(), m_GPUSceneDataDescriptorLayout, nullptr);

	vkDestroyDescriptorSetLayout(CEngine::device(), m_DrawImageDescriptorLayout, nullptr);

	vkDestroyCommandPool(CEngine::device(), m_UploadContext._commandPool, nullptr);
	vkDestroyFence(CEngine::device(), m_UploadContext._uploadFence, nullptr);

	m_EngineTextures->destroy();
}

void CVulkanRenderer::draw() {
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
	const auto [swapchainImage, swapchainImageIndex] = m_EngineTextures->getSwapchain().getSwapchainImage(getFrameIndex());

	// Reset the current fences, done here so the swapchain acquire doesn't stall the engine
	m_EngineTextures->getSwapchain().reset(getFrameIndex());

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Flush temporary frame data
	getCurrentFrame().mResourceDeallocator.flush();

	// Reset descriptor allocators
	getCurrentFrame().mDescriptorAllocator.clear();

	//allocate a new uniform buffer for the scene data

	// GPU Scene is only needed in render TODO: (should put in frame)
	m_GPUSceneDataBuffer = getCurrentFrame().mFrameResourceAllocator.allocateBuffer(sizeof(GPUSceneData), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	//write the buffer
	auto* sceneUniformData = (GPUSceneData*)m_GPUSceneDataBuffer->GetMappedData();
	m_SceneData = *sceneUniformData;

	//create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = getCurrentFrame().mDescriptorAllocator.allocate(m_GPUSceneDataDescriptorLayout);

	SDescriptorWriter writer;
	writer.writeBuffer(0, m_GPUSceneDataBuffer->buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.updateSet(globalDescriptor);

	render(cmd);

	// Flush temporary frame data after it is no longer needed
	getCurrentFrame().mResourceDeallocator.flush();
	getCurrentFrame().mFrameResourceAllocator.flush();

	// Make the swapchain image into presentable mode
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	auto [width, height, depth] = m_EngineTextures->mDrawImage->mImageExtent;
	CVulkanUtils::copyImageToImage(cmd, m_EngineTextures->mDrawImage->mImage, swapchainImage, {width, height}, m_EngineTextures->getSwapchain().mSwapchain.extent);

	// Set swapchain layout so it can be used by ImGui
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// ImGui draws over the swapchain, so it needs to be after the copy
	renderImGui(cmd, m_EngineTextures->getSwapchain().mSwapchainImageViews[swapchainImageIndex]);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::transitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	m_EngineTextures->getSwapchain().submit(cmd, m_GraphicsQueue, getFrameIndex(), swapchainImageIndex);

	//increase the number of frames drawn
	m_FrameNumber++;
}

void CVulkanRenderer::renderImGui(VkCommandBuffer cmd, VkImageView inTargetImageView) {
	VkRenderingAttachmentInfo colorAttachment = CVulkanUtils::createAttachmentInfo(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo = CVulkanUtils::createRenderingInfo(m_EngineTextures->getSwapchain().mSwapchain.extent, &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);
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
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
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

	VkDescriptorPoolCreateInfo pool_info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = (uint32_t)std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};

	VK_CHECK(vkCreateDescriptorPool(CEngine::device(), &pool_info, nullptr, &m_ImGuiDescriptorPool));

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info {
		.Instance = CEngine::instance(),
		.PhysicalDevice = CEngine::physicalDevice(),
		.Device = CEngine::device(),
		.Queue = m_GraphicsQueue,
		.DescriptorPool = m_ImGuiDescriptorPool,
		.MinImageCount = 3,
		.ImageCount = 3,
		.UseDynamicRendering = true
	};

	//dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_EngineTextures->getSwapchain().mFormat;

	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplSDL3_InitForVulkan(CEngine::get().getWindow().mWindow);

	m_ResourceDeallocator.push(m_ImGuiDescriptorPool);
}

void CNullRenderer::render(VkCommandBuffer cmd) {
	CVulkanUtils::transitionImage(cmd, m_EngineTextures->mDrawImage->mImage,VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}
