#include "EngineTextures.h"

#include <array>

#include "DescriptorManager.h"
#include "Engine.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "ResourceManager.h"

void CEngineTextures::init() {

	// Ensure previous textures have been destroyed
	// This is in the case of screen resizing
	m_ResourceManager.flush();

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto extent = CEngine::get().getWindow().mExtent;

	mDrawImage = m_ResourceManager.allocateImage("Draw Image", {extent.x, extent.y, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, VK_IMAGE_ASPECT_COLOR_BIT, false);

	mDepthImage = m_ResourceManager.allocateImage("Depth Image", {extent.x, extent.y, 1}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false);
}

void CEngineTextures::reallocate(const bool inUseVSync) {

	auto extent = CEngine::get().getWindow().mExtent;
	msgs("Reallocating Engine Textures to ({}, {})", extent.x, extent.y);

	m_Swapchain.recreate(inUseVSync);

	init();
}

void CEngineTextures::destroy() {

	m_ResourceManager.flush();

	m_Swapchain.cleanup();
}

