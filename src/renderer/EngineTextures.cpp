#include "EngineTextures.h"

#include <array>

#include "DescriptorManager.h"
#include "Engine.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "ResourceManager.h"

CEngineTextures::CEngineTextures() {
	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	// This is in the case of screen resizing
	m_ResourceManager.flush();

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto extent = CEngine::get().getWindow().mExtent;

	mDrawImage = m_ResourceManager.allocateImage({extent.x, extent.y, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, VK_IMAGE_ASPECT_COLOR_BIT, false);

	//make the descriptor set layout for our compute draw
	{
		SDescriptorLayoutBuilder builder;
		builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		mDrawImageDescriptorLayout = builder.build(VK_SHADER_STAGE_COMPUTE_BIT);

		CEngine::renderer().mGlobalResourceManager.push(mDrawImageDescriptorLayout);

		mDrawImageDescriptors = CEngine::renderer().mGlobalDescriptorAllocator.allocate(mDrawImageDescriptorLayout);

		SDescriptorWriter writer;
		writer.writeImage(0, mDrawImage->mImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.updateSet(mDrawImageDescriptors);
	}

	mDepthImage = m_ResourceManager.allocateImage({extent.x, extent.y, 1}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false);
}

void CEngineTextures::reallocate(const bool inUseVSync) {

	auto extent = CEngine::get().getWindow().mExtent;
	msgs("Reallocating Engine Textures to ({}, {})", extent.x, extent.y);

	m_Swapchain.recreate(inUseVSync);

	initializeTextures();
}

void CEngineTextures::destroy() {

	m_ResourceManager.flush();

	m_Swapchain.cleanup();
}

