#include "EngineTextures.h"

#include <array>

#include "Engine.h"
#include "VulkanDevice.h"
#include "ResourceAllocator.h"

CEngineTextures::CEngineTextures() {
	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	// This is in the case of screen resizing
	m_ResourceAllocator.flush();

	VkImageUsageFlags drawImageUsages = 0;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto [width, height] = CEngine::get().getWindow().mExtent;

	mDrawImage = m_ResourceAllocator.allocateImage({width, height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, VK_IMAGE_ASPECT_COLOR_BIT, false);

	mDepthImage = m_ResourceAllocator.allocateImage({width, height, 1}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false);
}

void CEngineTextures::reallocate() {

	auto [x, y] = CEngine::get().getWindow().mExtent;
	msgs("Reallocating Engine Textures to ({}, {})", x, y);

	m_Swapchain.recreate();

	initializeTextures();
}

void CEngineTextures::destroy() {

	m_ResourceAllocator.flush();

	m_Swapchain.cleanup();
}

