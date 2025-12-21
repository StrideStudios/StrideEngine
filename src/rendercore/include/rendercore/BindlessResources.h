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
class CBindlessResources final : public SObject, public IInitializable {

	REGISTER_CLASS(CBindlessResources, SObject)
	MAKE_LAZY_SINGLETON(CBindlessResources)

public:

	virtual void init() override {
		// Create Descriptor pool
		{
			auto poolSizes = {
				VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, gMaxTextures},
				VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, gMaxSamplers},
				VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, gMaxUniformBuffers},
				VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, gMaxStorageBuffers},
			};

			VkDescriptorPoolCreateInfo poolCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
				.maxSets = gMaxTextures + gMaxSamplers + gMaxUniformBuffers + gMaxStorageBuffers,
				.poolSizeCount = (uint32)poolSizes.size(),
				.pPoolSizes = poolSizes.begin()
			};

			CResourceManager::get().create(mDescriptorPool, poolCreateInfo);
		}

		// Create Descriptor Set layout
		{
			constexpr VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT; //VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT

			const auto inputFlags = {flags, flags, flags, flags};

	        auto binding = {
        		VkDescriptorSetLayoutBinding {
        			.binding = gTextureBinding,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
					.descriptorCount = gMaxTextures,
					.stageFlags = VK_SHADER_STAGE_ALL,
        		},
				VkDescriptorSetLayoutBinding {
					.binding = gSamplerBinding,
					.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
					.descriptorCount = gMaxSamplers,
					.stageFlags = VK_SHADER_STAGE_ALL,
				},
				VkDescriptorSetLayoutBinding {
					.binding = gUBOBinding,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = gMaxUniformBuffers,
					.stageFlags = VK_SHADER_STAGE_ALL,
				},
				VkDescriptorSetLayoutBinding {
					.binding = gSSBOBinding,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = gMaxStorageBuffers,
					.stageFlags = VK_SHADER_STAGE_ALL,
				}
	        };

			const auto flagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = (uint32)inputFlags.size(),
				.pBindingFlags = inputFlags.begin(),
			};

	        VkDescriptorSetLayoutCreateInfo layoutCreateInfo {
        		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        		.pNext = &flagInfo,
        		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        		.bindingCount = (uint32)binding.size(),
        		.pBindings = binding.begin(),
	        };

			CResourceManager::get().create(mDescriptorSetLayout, layoutCreateInfo);
		}

		{
			constexpr uint32 maxBinding = gMaxStorageBuffers - 1;
			VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
				.descriptorSetCount = 1,
				.pDescriptorCounts = &maxBinding
			};

			VkDescriptorSetAllocateInfo allocationCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = &countInfo,
				.descriptorPool = mDescriptorPool->mDescriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &mDescriptorSetLayout->mDescriptorSetLayout
			};

			CResourceManager::get().create<CDescriptorSet>(mDescriptorSet, allocationCreateInfo);

		}

		// Create Basic Pipeline Layout
		// This includes the 8 Vector4f Push Constants (128 bytes) and the global DescriptorSetLayout
		{
			auto pushConstants = {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(SPushConstants)
				}
			};

			VkPipelineLayoutCreateInfo layoutCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.setLayoutCount = 1,
				.pSetLayouts = &mDescriptorSetLayout->mDescriptorSetLayout,
				.pushConstantRangeCount = (uint32)pushConstants.size(),
				.pPushConstantRanges = pushConstants.begin()
			};

			CResourceManager::get().create(mPipelineLayout, layoutCreateInfo);
		}
	}

	static CDescriptorPool*& getBindlessDescriptorPool() {
		return get()->mDescriptorPool;
	}

	static CPipelineLayout*& getBasicPipelineLayout() {
		return get()->mPipelineLayout;
	}

	static CDescriptorSetLayout*& getBindlessDescriptorSetLayout() {
		return get()->mDescriptorSetLayout;
	}

	static CDescriptorSet*& getBindlessDescriptorSet() {
		return get()->mDescriptorSet;
	}

private:

	CDescriptorPool* mDescriptorPool = nullptr;
	CPipelineLayout* mPipelineLayout = nullptr;
	CDescriptorSetLayout* mDescriptorSetLayout = nullptr;
	CDescriptorSet* mDescriptorSet = nullptr;

};