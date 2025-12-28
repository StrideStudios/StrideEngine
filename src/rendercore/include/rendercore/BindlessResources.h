#pragma once

#include "rendercore/Material.h" //TODO: move PushConstants
#include "rendercore/VulkanResources.h"

// More than 65535 textures should not be needed, but more than 255 might be.
constexpr static uint32 gMaxTextures = std::numeric_limits<uint16>::max();
// There are very few types of samplers, so only 32 are needed
constexpr static uint32 gMaxSamplers = 32;
// Uniform buffers tend to be fast to access but very small, max of 255
// OpenGL spec states that uniform buffers guarantee up to 16 KB
// (Subtracted by gMaxSamplers to keep alignment)
constexpr static uint32 gMaxUniformBuffers = std::numeric_limits<uint8>::max() - gMaxSamplers;
// Shader Storage Buffers tend to be slower but larger
// OpenGL spec states that SSBOs guarantee up to 128 MB, but can be larger
constexpr static uint32 gMaxStorageBuffers = std::numeric_limits<uint16>::max();

static uint32 gTextureBinding = 0;
static uint32 gSamplerBinding = 1;
static uint32 gUBOBinding = 2;
static uint32 gSSBOBinding = 3;

// A simple holder for bindless resources
class CBindlessResources final : public SObject {

	REGISTER_CLASS(CBindlessResources, SObject)
	MAKE_LAZY_SINGLETON(CBindlessResources)

public:

	EXPORT virtual void init(const TShared<CVulkanDevice>& inDevice);

	EXPORT virtual void destroy();

	static TUnique<CDescriptorPool>& getBindlessDescriptorPool() {
		return get()->mDescriptorPool;
	}

	static TUnique<CPipelineLayout>& getBasicPipelineLayout() {
		return get()->mPipelineLayout;
	}

	static TUnique<CDescriptorSetLayout>& getBindlessDescriptorSetLayout() {
		return get()->mDescriptorSetLayout;
	}

	static TUnique<CDescriptorSet>& getBindlessDescriptorSet() {
		return get()->mDescriptorSet;
	}

private:

	TUnique<CDescriptorPool> mDescriptorPool = nullptr;
	TUnique<CPipelineLayout> mPipelineLayout = nullptr;
	TUnique<CDescriptorSetLayout> mDescriptorSetLayout = nullptr;
	TUnique<CDescriptorSet> mDescriptorSet = nullptr;

};