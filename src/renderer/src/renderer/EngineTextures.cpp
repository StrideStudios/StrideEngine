#include "renderer/EngineTextures.h"

#include <array>

#include "Engine.h"
#include "Material.h"
#include "renderer/VulkanDevice.h"
#include "renderer/VulkanRenderer.h"
#include "renderer/Swapchain.h"
#include "Viewport.h"

static CVulkanResourceManager gTexturesResourceManager;

void CEngineTextures::init() {

	CVulkanRenderer::get()->getResourceManager().create(m_Swapchain);

	// Initialize samplers
	// Default samplers repeat and do not have anisotropy
	{
		VkSamplerCreateInfo samplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT
		};

		samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

		CSampler* samplerNearest;
		CVulkanRenderer::get()->getResourceManager().create(samplerNearest, samplerCreateInfo);

		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		CSampler* samplerLinear;
		CVulkanRenderer::get()->getResourceManager().create(samplerLinear, samplerCreateInfo);

		const auto imageDescriptorInfo = VkDescriptorImageInfo{
			.sampler = *samplerNearest};

		const auto writeSet = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = CVulkanResourceManager::getBindlessDescriptorSet(),
			.dstBinding = gSamplerBinding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = &imageDescriptorInfo,
		};

		const auto imageDescriptorInfo2 = VkDescriptorImageInfo{
			.sampler = *samplerLinear};

		const auto writeSet2 = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = CVulkanResourceManager::getBindlessDescriptorSet(),
			.dstBinding = gSamplerBinding,
			.dstArrayElement = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = &imageDescriptorInfo2,
		};

		const auto sets = {writeSet, writeSet2};

		vkUpdateDescriptorSets(CRenderer::device(), (uint32)sets.size(), sets.begin(), 0, nullptr);
	}

	// Error checkerboard image
	const uint32 red = glm::packUnorm4x8(glm::vec4(1, 0, 0, 1));
	const uint32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
	std::array<uint32, 16 * 16> pixels{}; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = (x % 2 ^ y % 2) ? red : black;
		}
	}

	constexpr VkExtent3D extent(16, 16, 1);
	constexpr int32 size = extent.width * extent.height * extent.depth * 4;
	mErrorCheckerboardImage = CVulkanRenderer::get()->getResourceManager().allocateImage(pixels.data(), size, "Default Error", extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	{
		mErrorMaterial = std::make_shared<CMaterial>();
		mErrorMaterial->mShouldSave = false;
		mErrorMaterial->mName = "Error";
		mErrorMaterial->mPassType = EMaterialPass::ERROR;
	}

	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	// This is in the case of screen resizing
	gTexturesResourceManager.flush();

	constexpr VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const auto extent = CEngineViewport::get().mExtent;

	mDrawImage = gTexturesResourceManager.allocateImage("Draw Image", {extent.x, extent.y, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, VK_IMAGE_ASPECT_COLOR_BIT, false);

	mDepthImage = gTexturesResourceManager.allocateImage("Depth Image", {extent.x, extent.y, 1}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, false);
}

void CEngineTextures::reallocate(const bool inUseVSync) {

	auto extent = CEngineViewport::get().mExtent;
	msgs("Reallocating Engine Textures to ({}, {})", extent.x, extent.y);

	m_Swapchain->recreate(inUseVSync);

	initializeTextures();
}

void CEngineTextures::destroy() {
	gTexturesResourceManager.flush();
}

