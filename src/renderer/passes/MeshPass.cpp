#include "include/MeshPass.h"

#include "Engine.h"
#include "EngineBuffers.h"
#include "EngineTextures.h"
#include "GpuScene.h"
#include "PipelineBuilder.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "ShaderCompiler.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"
#include "EngineSettings.h"

#define COMMAND_CATEGORY "Rendering"
ADD_TEXT(Meshes, "Meshes: ");
ADD_TEXT(Drawcalls, "Draw Calls: ");
ADD_TEXT(Vertices, "Vertices: ");
ADD_TEXT(Triangles, "Triangles: ");
#undef COMMAND_CATEGORY

//TODO: for now this is hard coded base pass, dont need anything else for now
void CMeshPass::init(const EMeshPass inPassType) {
	passType = inPassType;

	CVulkanRenderer& renderer = CEngine::renderer();
	CGPUScene* gpuScene = renderer.mGPUScene;

	SShader frag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(), "material\\mesh.frag", frag));

	SShader errorFrag {
		.mStage = EShaderStage::PIXEL
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(), "material\\mesh_error.frag", errorFrag));

	SShader vert {
		.mStage = EShaderStage::VERTEX
	};
	VK_CHECK(CShaderCompiler::getShader(CEngine::device(),"material\\mesh.vert", vert))

	auto pushConstants = {
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SPushConstants)
		}
	};

	//TODO: global scene data? (avoid gpuscene input)
    VkDescriptorSetLayout layouts[] = {
		gpuScene->m_GPUSceneDataDescriptorLayout,
		CResourceManager::getBindlessDescriptorSetLayout()
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.setLayoutCount = 2,
		.pSetLayouts = layouts,
		.pushConstantRangeCount = (uint32)pushConstants.size(),
		.pPushConstantRanges = pushConstants.begin()
	};

	auto newLayout = renderer.mGlobalResourceManager.allocatePipelineLayout(layoutCreateInfo);

    opaquePipeline.layout = newLayout;
	errorPipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

	// Set shader modules and standard pipeline
	CPipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(vert.mModule, frag.mModule);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setNoMultisampling();
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// Set color and depth format
	pipelineBuilder.setColorAttachementFormat(renderer.mEngineTextures->mDrawImage->mImageFormat);
	pipelineBuilder.setDepthFormat(renderer.mEngineTextures->mDepthImage->mImageFormat);

	pipelineBuilder.m_PipelineLayout = newLayout;

	opaquePipeline.pipeline = renderer.mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	// Transparent should be additive and always render in front
	pipelineBuilder.enableBlendingAdditive();
	pipelineBuilder.depthTestAlwaysInFront();

	transparentPipeline.pipeline = renderer.mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	pipelineBuilder.setShaders(vert.mModule, errorFrag.mModule);
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	errorPipeline.pipeline = renderer.mGlobalResourceManager.allocatePipeline(pipelineBuilder);

	vkDestroyShaderModule(CEngine::device(), frag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), errorFrag.mModule, nullptr);
	vkDestroyShaderModule(CEngine::device(), vert.mModule, nullptr);
}

//TODO: probably faster with gpu
bool isVisible(const std::shared_ptr<SStaticMesh>& obj, const Matrix4f& viewproj) {
	constexpr static std::array corners {
		Vector3f { 1, 1, 1 },
		Vector3f { 1, 1, -1 },
		Vector3f { 1, -1, 1 },
		Vector3f { 1, -1, -1 },
		Vector3f { -1, 1, 1 },
		Vector3f { -1, 1, -1 },
		Vector3f { -1, -1, 1 },
		Vector3f { -1, -1, -1 },
	};

	const Matrix4f matrix = viewproj;// * obj.transform;

	Vector3f min = { 1.5, 1.5, 1.5 };
	Vector3f max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		Vector4f v = matrix * Vector4f(obj->bounds.origin + (corners[c] * obj->bounds.extents), 1.f);

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

void CMeshPass::render(const VkCommandBuffer cmd) {
	const CVulkanRenderer& renderer = CEngine::renderer();
	const CGPUScene* scene = renderer.mGPUScene;

	std::vector<std::shared_ptr<SStaticMesh>> renderObjects;

	{
		ZoneScopedN("Frustum Culling");
		for (auto obj : objects) {
			if (isVisible(obj, scene->m_GPUSceneData.viewProj)) {
				renderObjects.push_back(obj);
			}
		}
	}

	// Set number of meshes being drawn
	Meshes.setText(fmts("Meshes: {}", renderObjects.size()));

	// sort the opaque surfaces by material and mesh
	{
		ZoneScopedN("Sort Draws");
		std::ranges::sort(renderObjects, [&](const std::shared_ptr<SStaticMesh>& lhs, const std::shared_ptr<SStaticMesh>& rhs) {
			return lhs->meshBuffers->indexBuffer->buffer < rhs->meshBuffers->indexBuffer->buffer;
		});
	}

	//defined outside of the draw function, this is the state we will try to skip
	SMaterialPipeline* lastPipeline = nullptr;
	std::shared_ptr<CMaterial> lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	VkExtent2D extent {
		renderer.mEngineTextures->mDrawImage->mImageExtent.width,
		renderer.mEngineTextures->mDrawImage->mImageExtent.height
	};

	auto render = [&](const std::shared_ptr<SStaticMesh>& obj) {
		ZoneScoped;
		ZoneName(obj->name.c_str(), obj->name.size());

		// Rebind index buffer if starting a new mesh
		if (lastIndexBuffer != obj->meshBuffers->indexBuffer->buffer) {
			lastIndexBuffer = obj->meshBuffers->indexBuffer->buffer;
			vkCmdBindIndexBuffer(cmd, obj->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			constexpr VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &obj->meshBuffers->vertexBuffer->buffer, &offset);
		}

		// Loop through surfaces and render
		for (const auto& surface : obj->surfaces) {

			//TODO: auto pipeline rebind (or something)
			// If the materials arent the same, rebind material data
			SMaterialPipeline& pipeline = surface.material->getPipeline(renderer);
			if (surface.material != lastMaterial) {
				// If the pipeline has changed, rebind pipeline data
				if (&pipeline != lastPipeline) {
					lastPipeline = &pipeline;
					CResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
					CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, renderer.mGPUScene->m_Frames[renderer.getFrameIndex()].sceneDescriptor);
					CResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, CResourceManager::getBindlessDescriptorSet());

					CEngine::renderer().mViewport.update({extent.width, extent.height});
					CEngine::renderer().mViewport.set(cmd);
				}
			}

			vkCmdPushConstants(cmd, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), surface.material->mConstants.data());

			vkCmdDrawIndexed(cmd, surface.count, 1, surface.startIndex, 0, 0);

			drawCallCount++;
			vertexCount += surface.count;
		}
	};

	ZoneScopedN("Base Pass");

	/*for (const SStaticMesh& draw : m_MainRenderContext.opaqueSurfaces) {
		render(draw);
	}*/

	for (const auto& obj : renderObjects) {
		render(obj);
	}

	// Set number of drawcalls, vertices, and triangles
	Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	Vertices.setText(fmts("Vertices: {}", vertexCount));
	Triangles.setText(fmts("Triangles: {}", vertexCount / 3));
}
