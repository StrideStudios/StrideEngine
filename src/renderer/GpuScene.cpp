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

#define COMMAND_CATEGORY "Rendering"
ADD_TEXT(Meshes, "Meshes: ");
ADD_TEXT(Drawcalls, "Draw Calls: ");
ADD_TEXT(Vertices, "Vertices: ");
ADD_TEXT(Triangles, "Triangles: ");
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

	auto pushConstants = {
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(SGPUDrawPushConstants)
		},
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 72,
			.size = sizeof(uint32)
		}
	};

    SDescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    materialLayout = layoutBuilder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	CEngine::renderer().mGlobalResourceManager.push(materialLayout);

	VkDescriptorSetLayout layouts[] = {
		gpuScene->m_GPUSceneDataDescriptorLayout,
        materialLayout,
		CResourceManager::getBindlessDescriptorSetLayout()
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 3,
		.pSetLayouts = layouts,
		.pushConstantRangeCount = (uint32)pushConstants.size(),
		.pPushConstantRanges = pushConstants.begin()
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
	pipelineBuilder.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
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

	//pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	pipelineBuilder.disableDepthTest();

	transparentPipeline.pipeline = renderer->mGlobalResourceManager.allocatePipeline(pipelineBuilder);

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
		def.bounds = s.bounds;
		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers->vertexBufferAddress;

		if (def.material->passType == EMaterialPass::TRANSLUCENT) {
			ctx.transparentSurfaces.push_back(def);
		} else {
			ctx.opaqueSurfaces.push_back(def);
		}
	}

	// recurse down
	SNode::render(topMatrix, ctx);
}

void SLoadedGLTF::render(const glm::mat4 &topMatrix, SRenderContext &ctx) {
	// create renderables from the scenenodes
	for (auto& n : topNodes) {
		n->render(topMatrix, ctx);
	}
}

void SLoadedGLTF::clearAll() {
	descriptorPool.destroy();
}

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

	uint32 white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
	m_WhiteImage = renderer->mGlobalResourceManager.allocateImage(&white, "Default White", {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
	m_GreyImage = renderer->mGlobalResourceManager.allocateImage(&grey, "Default Grey", {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	uint32 black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
	m_BlackImage = renderer->mGlobalResourceManager.allocateImage(&black, "Default Black", {1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	//checkerboard image
	uint32 magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	m_ErrorCheckerboardImage = renderer->mGlobalResourceManager.allocateImage(pixels.data(), "Default Error", VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

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
	sceneUniformData->colorFactors = {1.f,1.f,1.f,1.f};
	sceneUniformData->metal_rough_factors = {1.f,0.5f,0.f,0.f};

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

	std::string structurePath = { "structure.glb" };
	auto structureFile = CMeshLoader::loadGLTF(renderer, this, structurePath);

	assert(structureFile.has_value());

	m_LoadedScenes["structure"] = *structureFile;
}

//TODO: probably faster with gpu
bool isVisible(const SRenderObject& obj, const Matrix4f& viewproj) {
	std::array corners {
		Vector3f { 1, 1, 1 },
		Vector3f { 1, 1, -1 },
		Vector3f { 1, -1, 1 },
		Vector3f { 1, -1, -1 },
		Vector3f { -1, 1, 1 },
		Vector3f { -1, 1, -1 },
		Vector3f { -1, -1, 1 },
		Vector3f { -1, -1, -1 },
	};

	Matrix4f matrix = viewproj * obj.transform;

	Vector3f min = { 1.5, 1.5, 1.5 };
	Vector3f max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		Vector4f v = matrix * Vector4f(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

		// perspective correction
		v.x = v.x / v.w;
		v.y = v.y / v.w;
		v.z = v.z / v.w;

		min = glm::min(Vector3f { v.x, v.y, v.z }, min);
		max = glm::max(Vector3f { v.x, v.y, v.z }, max);
	}

	// check the clip space box is within the view
	if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f) {
		return false;
	}
	return true;
}


void CGPUScene::render(CVulkanRenderer* renderer, VkCommandBuffer cmd) {
	update(renderer);

	std::vector<uint32_t> opaque_draws;
	opaque_draws.reserve(m_MainRenderContext.opaqueSurfaces.size());

	{
		ZoneScopedN("Frustrum Culling");
		for (uint32_t i = 0; i < m_MainRenderContext.opaqueSurfaces.size(); i++) {
			if (isVisible(m_MainRenderContext.opaqueSurfaces[i], m_GPUSceneData.viewProj)) {
				opaque_draws.push_back(i);
			}
		}
	}

	// Set number of meshes being drawn
	Meshes.setText(fmts("Meshes: {}", opaque_draws.size()));

	// sort the opaque surfaces by material and mesh
	{
		ZoneScopedN("Sort Draws");
		std::sort(opaque_draws.begin(), opaque_draws.end(), [&](const auto& iA, const auto& iB) {
			const SRenderObject& A = m_MainRenderContext.opaqueSurfaces[iA];
			const SRenderObject& B = m_MainRenderContext.opaqueSurfaces[iB];
			if (A.material == B.material) {
				return A.indexBuffer < B.indexBuffer;
			}
			return A.material < B.material;
		});
	}

	//defined outside of the draw function, this is the state we will try to skip
	SMaterialPipeline* lastPipeline = nullptr;
	SMaterialInstance* lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	auto render = [&](const SRenderObject& draw) {
		if (draw.material != lastMaterial) {
			lastMaterial = draw.material;

			VkExtent2D extent {
				renderer->mEngineTextures->mDrawImage->mImageExtent.width,
				renderer->mEngineTextures->mDrawImage->mImageExtent.height
			};
			//rebind pipeline and descriptors if the material changed
			if (draw.material->pipeline != lastPipeline) {

				lastPipeline = draw.material->pipeline;
				CResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
				CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1, m_Frames[renderer->getFrameIndex()].sceneDescriptor);
				CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 2, 1, CResourceManager::getBindlessDescriptorSet());

				CEngine::renderer().mViewport.update({extent.width, extent.height});
				CEngine::renderer().mViewport.set(cmd);
			}
			CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1, draw.material->materialSet);
		}

		//rebind index buffer if needed
		if (draw.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = draw.indexBuffer;
			vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		// calculate final mesh matrix
		SGPUDrawPushConstants push_constants;
		push_constants.worldMatrix = draw.transform;
		push_constants.vertexBuffer = draw.vertexBufferAddress;

		vkCmdPushConstants(cmd, draw.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SGPUDrawPushConstants), &push_constants);
		auto i = { uint32(0) };
		vkCmdPushConstants(cmd, draw.material->pipeline->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 72, sizeof(uint32), i.begin());

		vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, 0, 0);

		drawCallCount++;
		vertexCount += draw.indexCount;
	};

	ZoneScopedN("Base Pass");

	/*for (const SRenderObject& draw : m_MainRenderContext.opaqueSurfaces) {
		render(draw);
	}*/

	for (const auto& r : opaque_draws) {
		render(m_MainRenderContext.opaqueSurfaces[r]);
	}

	for (const SRenderObject& draw : m_MainRenderContext.transparentSurfaces) {
		render(draw);
	}

	// Set number of drawcalls, vertices, and triangles
	Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	Vertices.setText(fmts("Vertices: {}", vertexCount));
	Triangles.setText(fmts("Triangles: {}", vertexCount / 3));

}

void CGPUScene::update(CVulkanRenderer* renderer) {
	ZoneScopedN("GPUScene Update");

	m_MainRenderContext.opaqueSurfaces.clear();
	m_MainRenderContext.transparentSurfaces.clear();

	m_LoadedScenes["structure"]->render(glm::mat4{ 1.f }, m_MainRenderContext);

	CEngine::get().mMainCamera.update();

	const auto [x, y, z] = renderer->mEngineTextures->mDrawImage->mImageExtent;
	Matrix4f view = CEngine::get().mMainCamera.getViewMatrix();
	// camera projection
	float tanHalfFovy = tan(glm::radians(CEngine::get().mMainCamera.mFOV) / 2.f);
	float aspect = (float)x / (float)y;

	Matrix4f projection(0.f);
	projection[0][0] = 1.f / (aspect * tanHalfFovy);
	projection[1][1] = 1.f / tanHalfFovy;
	projection[2][3] = - 1.f;
	projection[3][2] = 0.1f;

	m_GPUSceneData.view = view;
	// camera projection
	m_GPUSceneData.proj = projection;

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	m_GPUSceneData.proj[1][1] *= -1;
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