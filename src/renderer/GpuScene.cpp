#include "GpuScene.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "DescriptorManager.h"
#include "Engine.h"
#include "EngineBuffers.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "PipelineBuilder.h"
#include "MeshLoader.h"
#include "ResourceManager.h"
#include "ShaderCompiler.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"

CGPUScene::CGPUScene(CVulkanRenderer* renderer) {

	VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
	};

	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;

	m_DefaultSamplerNearest = renderer->mGlobalResourceManager.allocateSampler(samplerCreateInfo);

	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	m_DefaultSamplerLinear = renderer->mGlobalResourceManager.allocateSampler(samplerCreateInfo);

	const auto imageDescriptorInfo = VkDescriptorImageInfo{
		.sampler = m_DefaultSamplerNearest};

	const auto writeSet = VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = CResourceManager::getBindlessDescriptorSet(),
		.dstBinding = gSamplerBinding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo = &imageDescriptorInfo,
	};

	const auto imageDescriptorInfo2 = VkDescriptorImageInfo{
		.sampler = m_DefaultSamplerLinear};

	const auto writeSet2 = VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = CResourceManager::getBindlessDescriptorSet(),
		.dstBinding = gSamplerBinding,
		.dstArrayElement = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo = &imageDescriptorInfo2,
	};

	auto sets = {writeSet, writeSet2};

	vkUpdateDescriptorSets(CEngine::device(), (uint32)sets.size(), sets.begin(), 0, nullptr);


	//checkerboard image
	uint32 red = glm::packUnorm4x8(glm::vec4(1, 0, 0, 1));
	uint32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	std::array<uint32, 16 * 16> pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = (x % 2 ^ y % 2) ? red : black;
		}
	}

	constexpr VkExtent3D extent(16, 16, 1);
	constexpr int32 size = extent.width * extent.height * extent.depth * 4;
	m_ErrorCheckerboardImage = renderer->mGlobalResourceManager.allocateImage(pixels.data(), size, "Default Error", extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	{
		mErrorMaterial = std::make_shared<CMaterial>();
		mErrorMaterial->mShouldSave = false;
		mErrorMaterial->mName = "Error";
		mErrorMaterial->mPassType = EMaterialPass::ERROR;
	}

	SDescriptorLayoutBuilder builder;
	builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	m_GPUSceneDataDescriptorLayout = builder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	renderer->mGlobalResourceManager.push(m_GPUSceneDataDescriptorLayout);

	basePass.build(renderer, this);
}

void CGPUScene::render(CVulkanRenderer* renderer, VkCommandBuffer cmd) {
	update(renderer);

	basePass.render(renderer, cmd);

}

void CGPUScene::update(CVulkanRenderer* renderer) {
	ZoneScopedN("GPUScene Update");

	CEngine::get().mMainCamera.update();

	// Update Scene Data Buffer
	{
		m_GPUSceneData.view = CEngine::get().mMainCamera.mViewMatrix;
		// camera projection
		m_GPUSceneData.proj = CEngine::get().mMainCamera.mProjectionMatrix;

		m_GPUSceneData.viewProj = CEngine::get().mMainCamera.mViewProjectionMatrix;

		//some default lighting parameters
		m_GPUSceneData.ambientColor = glm::vec4(.1f);
		m_GPUSceneData.sunlightColor = glm::vec4(1.f);
		m_GPUSceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);

		//allocate a new uniform buffer for the scene data
		m_GPUSceneDataBuffer = renderer->getCurrentFrame().mFrameResourceManager.allocateBuffer(sizeof(Data), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	}

	//write the buffer
	auto* sceneUniformData = static_cast<Data*>(m_GPUSceneDataBuffer->GetMappedData());
	*sceneUniformData = m_GPUSceneData;

	//create a descriptor set that binds that buffer and update it
	m_Frames[renderer->getFrameIndex()].sceneDescriptor = renderer->getCurrentFrame().mDescriptorAllocator.allocate(m_GPUSceneDataDescriptorLayout);

	SDescriptorWriter writer;
	writer.writeBuffer(0, m_GPUSceneDataBuffer->buffer, sizeof(Data), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.updateSet(m_Frames[renderer->getFrameIndex()].sceneDescriptor);
}