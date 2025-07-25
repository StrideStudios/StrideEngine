#include "GpuScene.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "Engine.h"
#include "EngineBuffers.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "GraphicsRenderer.h"
#include "ResourceManager.h"
#include "ShaderCompiler.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

#define COMMAND_CATEGORY "Camera"
ADD_COMMAND(Vector3f, Translation, {0.f, -5.f, 0.f});
ADD_COMMAND(Vector3f, Rotation, {0.f, 0.f, 0.f});
#undef COMMAND_CATEGORY

void SGLTFMetallic_Roughness::buildPipelines(CVulkanRenderer* renderer, CGPUScene* gpuScene) {

	SShader frag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(), "material\\mesh.frag", frag));

	SShader vert {
		.mStage = EShaderStage::VERTEX
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(),"material\\mesh.vert", vert))

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(SGPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    SDescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	renderer->mGlobalResourceManager.push(materialLayout);

	VkDescriptorSetLayout layouts[] = {
		gpuScene->m_GPUSceneDataDescriptorLayout,
        materialLayout
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 2,
		.pSetLayouts = layouts,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &matrixRange
	};

	auto newLayout = renderer->mGlobalResourceManager.allocatePipelineLayout(layoutCreateInfo);

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	CPipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(vert.mModule, frag.mModule);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setNoMultisampling();
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.setColorAttachementFormat(renderer->mEngineTextures->mDrawImage->mImageFormat);
	pipelineBuilder.setDepthFormat(renderer->mEngineTextures->mDepthImage->mImageFormat);

	// use the triangle layout we created
	pipelineBuilder.m_PipelineLayout = newLayout;

	// finally build the pipeline
	opaquePipeline.pipeline = renderer->mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	// create the transparent variant
	pipelineBuilder.enableBlendingAdditive();

	pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	opaquePipeline.pipeline = renderer->mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	vkDestroyShaderModule(CEngine::device(), frag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), vert.mModule, nullptr);
}

SMaterialInstance SGLTFMetallic_Roughness::writeMaterial(EMaterialPass pass, const MaterialResources& resources, SDescriptorAllocator& descriptorAllocator) {
	SMaterialInstance matData;
	matData.passType = pass;
	if (pass == EMaterialPass::TRANSLUCENT) {
		matData.pipeline = &transparentPipeline;
	} else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.allocate(materialLayout);

	writer.clear();
	writer.writeBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.writeImage(1, resources.colorImage->mImageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.writeImage(2, resources.metalRoughImage->mImageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.updateSet(matData.materialSet);

	return matData;
}

void SMeshNode::render(const Matrix4f &topMatrix, SRenderContext &ctx) {
	Matrix4f nodeMatrix = topMatrix * worldTransform;

	for (auto& s : mesh->surfaces) {
		SRenderObject def;
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = mesh->meshBuffers->indexBuffer->buffer;
		def.material = &s.material->data;

		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers->vertexBufferAddress;

		ctx.opaqueSurfaces.push_back(def);
	}

	// recurse down
	SNode::render(topMatrix, ctx);
}

CGPUScene::CGPUScene(CVulkanRenderer* renderer) {

	uint32 white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	m_WhiteImage = renderer->mGlobalResourceManager.allocateImage(&white, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	m_GreyImage = renderer->mGlobalResourceManager.allocateImage(&grey, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	m_BlackImage = renderer->mGlobalResourceManager.allocateImage(&white, {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	//checkerboard image
	uint32 magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	m_ErrorCheckerboardImage = renderer->mGlobalResourceManager.allocateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
	};

	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;

	m_DefaultSamplerNearest = renderer->mGlobalResourceManager.allocateSampler(samplerCreateInfo);

	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	m_DefaultSamplerLinear = renderer->mGlobalResourceManager.allocateSampler(samplerCreateInfo);

	SDescriptorLayoutBuilder builder;
	builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	m_GPUSceneDataDescriptorLayout = builder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	renderer->mGlobalResourceManager.push(m_GPUSceneDataDescriptorLayout);

	SGLTFMetallic_Roughness::MaterialResources materialResources;
	//default the material textures
	materialResources.colorImage = m_WhiteImage;
	materialResources.colorSampler = m_DefaultSamplerLinear;
	materialResources.metalRoughImage = m_WhiteImage;
	materialResources.metalRoughSampler = m_DefaultSamplerLinear;

	//set the uniform buffer for the material data
	const SBuffer& materialConstants = renderer->mGlobalResourceManager.allocateBuffer(sizeof(SGLTFMetallic_Roughness::MaterialConstants), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	//write the buffer
	const auto sceneUniformData = static_cast<SGLTFMetallic_Roughness::MaterialConstants *>(materialConstants->GetMappedData());
	sceneUniformData->colorFactors = glm::vec4{1,1,1,1};
	sceneUniformData->metal_rough_factors = glm::vec4{1,0.5,0,0};

	materialResources.dataBuffer = materialConstants->buffer;
	materialResources.dataBufferOffset = 0;

	metalRoughMaterial.buildPipelines(renderer, this);

	m_DefaultData = metalRoughMaterial.writeMaterial(EMaterialPass::OPAQUE,materialResources, renderer->mGlobalDescriptorAllocator);

	for (const auto& mesh : renderer->mEngineBuffers->testMeshes) {
		auto newNode = std::make_shared<SMeshNode>();
		newNode->mesh = mesh;

		newNode->localTransform = glm::mat4{ 1.f };
		newNode->worldTransform = glm::mat4{ 1.f };

		for (auto& surface : newNode->mesh->surfaces) {
			surface.material = std::make_shared<GLTFMaterial>(m_DefaultData);
		}

		m_LoadedNodes[mesh->name] = std::move(newNode);
	}
}

void CGPUScene::render(CVulkanRenderer* renderer, VkCommandBuffer cmd) {
	update(renderer);

	for (const SRenderObject& draw : m_MainRenderContext.opaqueSurfaces) {

		vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 0,1, &m_Frames[renderer->getFrameIndex()].sceneDescriptor,0,nullptr );
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 1,1, &draw.material->materialSet,0,nullptr );

		vkCmdBindIndexBuffer(cmd, draw.indexBuffer,0,VK_INDEX_TYPE_UINT32);

		SGPUDrawPushConstants pushConstants;
		pushConstants.vertexBuffer = draw.vertexBufferAddress;
		pushConstants.worldMatrix = draw.transform;
		vkCmdPushConstants(cmd,draw.material->pipeline->layout ,VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(SGPUDrawPushConstants), &pushConstants);

		vkCmdDrawIndexed(cmd,draw.indexCount,1,draw.firstIndex,0,0);
	}
}

void CGPUScene::update(CVulkanRenderer* renderer) {
	m_MainRenderContext.opaqueSurfaces.clear();

	m_LoadedNodes["Suzanne"]->render(glm::mat4(1.f), m_MainRenderContext);

	const auto [x, y, z] = renderer->mEngineTextures->mDrawImage->mImageExtent;
	glm::mat4 view = glm::translate(glm::mat4(1.f), { Translation.get().x,Translation.get().z,Translation.get().y }); // Swap y and z so z will be the up vector
	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)x / (float)y, 0.1f, 10000.f);

	m_GPUSceneData.view = view;
	// camera projection
	m_GPUSceneData.proj = projection;

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	m_GPUSceneData.proj.y.y *= -1;
	m_GPUSceneData.viewProj = m_GPUSceneData.proj * m_GPUSceneData.view;

	//some default lighting parameters
	m_GPUSceneData.ambientColor = glm::vec4(.1f);
	m_GPUSceneData.sunlightColor = glm::vec4(1.f);
	m_GPUSceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);

	//allocate a new uniform buffer for the scene data
	m_GPUSceneDataBuffer = renderer->getCurrentFrame().mFrameResourceManager.allocateBuffer(sizeof(Data), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	//write the buffer
	auto* sceneUniformData = static_cast<Data*>(m_GPUSceneDataBuffer->GetMappedData());
	*sceneUniformData = m_GPUSceneData;

	//create a descriptor set that binds that buffer and update it
	m_Frames[renderer->getFrameIndex()].sceneDescriptor = renderer->getCurrentFrame().mDescriptorAllocator.allocate(m_GPUSceneDataDescriptorLayout);

	SDescriptorWriter writer;
	writer.writeBuffer(0, m_GPUSceneDataBuffer->buffer, sizeof(Data), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.updateSet(m_Frames[renderer->getFrameIndex()].sceneDescriptor);
}