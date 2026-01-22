#include "renderer/EngineTextures.h"

#include <array>

#include "VkBootstrap.h"
#include "VRI/BindlessResources.h"
#include "engine/Engine.h"
#include "rendercore/Material.h"
#include "renderer/VulkanRenderer.h"
#include "engine/Viewport.h"

#include "VRI/VRICommands.h"
#include "VRI/VRISwapchain.h"

CEngineTextures::CEngineTextures(const TFrail<CRenderer>& renderer) {

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

		mNearestSampler = TUnique<CSampler>{samplerCreateInfo};

		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		mLinearSampler = TUnique<CSampler>{samplerCreateInfo};;

		const auto imageDescriptorInfo = VkDescriptorImageInfo{
			.sampler = *mNearestSampler};

		const auto writeSet = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = *CBindlessResources::getBindlessDescriptorSet(),
			.dstBinding = gSamplerBinding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = &imageDescriptorInfo,
		};

		const auto imageDescriptorInfo2 = VkDescriptorImageInfo{
			.sampler = *mLinearSampler};

		const auto writeSet2 = VkWriteDescriptorSet{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = *CBindlessResources::getBindlessDescriptorSet(),
			.dstBinding = gSamplerBinding,
			.dstArrayElement = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = &imageDescriptorInfo2,
		};

		const auto sets = {writeSet, writeSet2};

		vkUpdateDescriptorSets(CVRI::get()->getDevice()->device, (uint32)sets.size(), sets.begin(), 0, nullptr);
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
	mErrorCheckerboardImage = TUnique<SVRIImage>{"Default Error", extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT};

	renderer->immediateSubmit([&](const TFrail<CVRICommands>& cmd) {
		mErrorCheckerboardImage->push(cmd, pixels.data(), size);
	});

	{
		mErrorMaterial = TUnique<CMaterial>{};
		mErrorMaterial->mShouldSave = false;
		mErrorMaterial->mName = "Error";
		mErrorMaterial->mPassType = EMaterialPass::ERROR;
	}

	initializeTextures();
}

void CEngineTextures::initializeTextures() {

	// Ensure previous textures have been destroyed
	// This is in the case of screen resizing
	if (mDrawImage || mDepthImage) {
		mDrawImage.destroy();
		mDepthImage.destroy();
	}

	constexpr VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const auto extent = CEngine::get()->getViewport()->mExtent;

	mDrawImage = TUnique<SVRIImage>{"Draw Image", VkExtent3D{extent.x, extent.y, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, VK_IMAGE_ASPECT_COLOR_BIT};

	mDepthImage = TUnique<SVRIImage>{"Depth Image", VkExtent3D{extent.x, extent.y, 1}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT};
}

void CEngineTextures::reallocate(const SRendererInfo& info, const bool inUseVSync) {

	auto extent = CEngine::get()->getViewport()->mExtent;
	msgs("Reallocating Engine Textures to ({}, {})", extent.x, extent.y);

	CVRI::get()->getSwapchain()->recreate(inUseVSync);

	initializeTextures();
}

void CEngineTextures::destroy() {
	mErrorCheckerboardImage.destroy();
	if (mDrawImage || mDepthImage) {
		mDrawImage.destroy();
		mDepthImage.destroy();
	}
	mLinearSampler.destroy();
	mNearestSampler.destroy();
}

