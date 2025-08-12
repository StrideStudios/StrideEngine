#include "include/MeshPass.h"

#include "Engine.h"
#include "EngineTextures.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "tracy/Tracy.hpp"
#include "EngineSettings.h"
#include "Scene.h"
#include "Viewport.h"

#define SETTINGS_CATEGORY "Rendering"
ADD_TEXT(Meshes, "Meshes: ");
ADD_TEXT(Drawcalls, "Draw Calls: ");
ADD_TEXT(Vertices, "Vertices: ");
ADD_TEXT(Triangles, "Triangles: ");
#undef SETTINGS_CATEGORY

//TODO: for now this is hard coded base pass, dont need anything else for now
void CMeshPass::init(const EMeshPass inPassType) {
	passType = inPassType;

	CVulkanRenderer& renderer = CEngine::renderer();

	CVulkanResourceManager manager;

	const SShader frag = manager.getShader("material\\mesh.frag");
	const SShader errorFrag = manager.getShader("material\\mesh_error.frag");
	const SShader vert = manager.getShader("material\\mesh.vert");

	SPipelineCreateInfo createInfo {
		.vertexModule = *vert.mModule,
		.fragmentModule = *frag.mModule,
		.mColorFormat = renderer.mEngineTextures->mDrawImage->mImageFormat,
		.mDepthFormat = renderer.mEngineTextures->mDepthImage->mImageFormat
	};

	//TODO: could probably read from shader and do automatically...
	CVertexAttributeArchive attributes;
	attributes.createBinding(VK_VERTEX_INPUT_RATE_VERTEX);
	attributes << VK_FORMAT_R32G32B32_SFLOAT;// vec3 position
	attributes << VK_FORMAT_R32_UINT; // uint UV
	attributes << VK_FORMAT_R32G32B32_SFLOAT; // vec3 normal
	attributes << VK_FORMAT_R32_UINT;// uint color
	attributes.createBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;// mat4 Transform
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;
	attributes << VK_FORMAT_R32G32B32A32_SFLOAT;

	opaquePipeline = renderer.mGlobalResourceManager.allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	// Transparent should be additive and always render in front
	createInfo.mBlendMode = EBlendMode::ADDITIVE;
	createInfo.mDepthTestMode = EDepthTestMode::FRONT;

	transparentPipeline = renderer.mGlobalResourceManager.allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	createInfo.fragmentModule = *errorFrag.mModule;
	createInfo.mBlendMode = EBlendMode::NONE;
	createInfo.mDepthTestMode = EDepthTestMode::NORMAL;

	errorPipeline = renderer.mGlobalResourceManager.allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	manager.flush();
}

//TODO: probably faster with gpu
bool isVisible(const std::shared_ptr<IRenderable>& renderable, const Matrix4f& viewproj) {
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

	const Matrix4f matrix = viewproj * renderable->getTransformMatrix();

	Vector3f min = { 1.5, 1.5, 1.5 };
	Vector3f max = { -1.5, -1.5, -1.5 };

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		Vector4f v = matrix * Vector4f(renderable->getMesh()->bounds.origin + (corners[c] * renderable->getMesh()->bounds.extents), 1.f);

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
	ZoneScopedN("Base Pass");

	CVulkanRenderer& renderer = CEngine::renderer();

	std::map<std::shared_ptr<SStaticMesh>, std::vector<SInstance>> renderObjects;
	{
		ZoneScopedN("Frustum Culling");
		for (const auto& renderable : CScene::get().data.objects) {
			if (renderable) {
				if (auto renderableObject = std::dynamic_pointer_cast<IRenderable>(renderable); renderableObject && renderableObject->getMesh() && isVisible(renderableObject, renderer.mSceneData.mViewProj)) {
					if (renderObjects.contains(renderableObject->getMesh())) {
						renderObjects[renderableObject->getMesh()].push_back(SInstance{renderableObject->getTransformMatrix()});
					} else {
						renderObjects.emplace(renderableObject->getMesh(), std::vector{SInstance{renderableObject->getTransformMatrix()}});
					}
				}
			}
		}
	}

	// Set number of meshes being drawn
	Meshes.setText(fmts("Meshes: {}", renderObjects.size()));

	// Defined outside the draw function, this is the state we will try to skip
	CPipeline* lastPipeline = nullptr;
	std::shared_ptr<CMaterial> lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	for (const auto& [mesh, instance] : renderObjects) {
		const size_t NumInstances = instance.size();

		ZoneScoped;
		ZoneName(mesh->name.c_str(), mesh->name.size());

		// Rebind index buffer if starting a new mesh
		if (lastIndexBuffer != mesh->meshBuffers.indexBuffer->buffer) {
			ZoneScopedN("Bind Buffers");

			lastIndexBuffer = mesh->meshBuffers.indexBuffer->buffer;
			vkCmdBindIndexBuffer(cmd, mesh->meshBuffers.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			const auto offset = {
				VkDeviceSize { 0 },
				VkDeviceSize { 0 }
			};

			const SBuffer_T* instanceBuffer = renderer.getCurrentFrame().mFrameResourceManager.allocateBuffer(NumInstances * sizeof(SInstance), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

			void* data;
			instanceBuffer->mapData(&data);
			memcpy(data, instance.data(), NumInstances * sizeof(SInstance));
			instanceBuffer->unMapData();

			const auto buffers = {
				mesh->meshBuffers.vertexBuffer->buffer,
				instanceBuffer->buffer
			};
			vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
		}

		// Loop through surfaces and render
		for (const auto& surface : mesh->surfaces) {

			//TODO: auto pipeline rebind (or something)
			// If the materials arent the same, rebind material data
			CPipeline* pipeline = surface.material->getPipeline(renderer);
			if (surface.material != lastMaterial) {
				lastMaterial = surface.material;
				// If the pipeline has changed, rebind pipeline data
				if (pipeline != lastPipeline) {
					lastPipeline = pipeline;
					CVulkanResourceManager::bindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mPipeline);
					CVulkanResourceManager::bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->mLayout, 0, 1, CVulkanResourceManager::getBindlessDescriptorSet());

					//TODO: shouldnt do this here...
					CEngine::get().getViewport().set(cmd);
				}
			}

			vkCmdPushConstants(cmd, pipeline->mLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstants), surface.material->mConstants.data());

			vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

			drawCallCount++;
			vertexCount += surface.count * NumInstances;
		}
	}

	// Set number of drawcalls, vertices, and triangles
	Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	Vertices.setText(fmts("Vertices: {}", vertexCount));
	Triangles.setText(fmts("Triangles: {}", vertexCount / 3));
}
