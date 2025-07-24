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

CVulkanRenderer::CVulkanRenderer() {
	// Ensure the renderer is only created once
	astOnce(CVulkanRenderer);

	// Use vkb to get a Graphics queue
	m_GraphicsQueue = CEngine::get().getDevice().getDevice().get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = CEngine::get().getDevice().getDevice().get_queue_index(vkb::QueueType::graphics).value();

	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		const VkCommandPoolCreateInfo commandPoolInfo = CVulkanInfo::createCommandPoolInfo(
			m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (auto &frame: m_Frames) {
			VK_CHECK(vkCreateCommandPool(CEngine::get().getDevice().getDevice(), &commandPoolInfo, nullptr, &frame.mCommandPool));

			m_ResourceDeallocator.push(&frame.mCommandPool);

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = CVulkanInfo::createCommandAllocateInfo(frame.mCommandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(CEngine::get().getDevice().getDevice(), &cmdAllocInfo, &frame.mMainCommandBuffer));
		}
	}

	m_EngineTextures = std::make_unique<CEngineTextures>(this);

	//create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<SDescriptorAllocator::PoolSizeRatio> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
	};

	m_GlobalDescriptorAllocator.init(CEngine::get().getDevice().getDevice(), 10, sizes);

	//make the descriptor set layout for our compute draw
	{
		SDescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		m_DrawImageDescriptorLayout = builder.build(CEngine::get().getDevice().getDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
	}

	m_ResourceDeallocator.push(&m_GlobalDescriptorAllocator.mPool);

	initDescriptors();

	initImGui();
}

void CVulkanRenderer::destroy() {

	m_ResourceDeallocator.flush();

	for (auto & mFrame : m_Frames) {
		mFrame.mResourceDeallocator.flush();
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	m_DescriptorResourceDeallocator.flush();

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

	// Flush temporary frame data TODO: not sure when this should be used
	m_Frames[getFrameIndex()].mResourceDeallocator.flush();

	// Make the swapchain image into writeable mode before rendering
	CVulkanUtils::TransitionImage(cmd, m_EngineTextures->mDrawImage.mImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	render(cmd);

	// Make the swapchain image into presentable mode
	CVulkanUtils::TransitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Execute a copy from the draw image into the swapchain
	auto [width, height, depth] = m_EngineTextures->mDrawImage.mImageExtent;
	CVulkanUtils::CopyImageToImage(cmd, m_EngineTextures->mDrawImage.mImage, swapchainImage, {width, height}, m_EngineTextures->getSwapchain().mSwapchain.extent);

	// Set swapchain image layout to Present so we can show it on the screen
	CVulkanUtils::TransitionImage(cmd, swapchainImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// ImGui draws over the swapchain, so it needs to be after the copy
	renderImGui(cmd, m_EngineTextures->getSwapchain().mSwapchainImageViews[swapchainImageIndex]);

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
	vkDeviceWaitIdle(CEngine::get().getDevice().getDevice());

	m_EngineTextures->getSwapchain().wait(getFrameIndex());
}

void CVulkanRenderer::initDescriptors() {

	// Ensure previous descriptors are destroyed
	m_DescriptorResourceDeallocator.flush();

	//allocate a descriptor set for our draw image
	m_DrawImageDescriptors = m_GlobalDescriptorAllocator.allocate(CEngine::get().getDevice().getDevice(),m_DrawImageDescriptorLayout);

	updateDescriptors();

	//cleanup previous descriptor set layouts
	m_DescriptorResourceDeallocator.push(&m_DrawImageDescriptorLayout);
}

void CVulkanRenderer::updateDescriptors() {
	VkDescriptorImageInfo imgInfo {
		.imageView = m_EngineTextures->mDrawImage.mImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	VkWriteDescriptorSet drawImageWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,

		.dstSet = m_DrawImageDescriptors,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imgInfo
	};

	vkUpdateDescriptorSets(CEngine::get().getDevice().getDevice(), 1, &drawImageWrite, 0, nullptr);
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

	VK_CHECK(vkCreateDescriptorPool(CEngine::get().getDevice().getDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool));

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info {
		.Instance = CEngine::get().getDevice().getInstance(),
		.PhysicalDevice = CEngine::get().getDevice().getPhysicalDevice(),
		.Device = CEngine::get().getDevice().getDevice(),
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

	m_ResourceDeallocator.push(&m_ImGuiDescriptorPool);
}

