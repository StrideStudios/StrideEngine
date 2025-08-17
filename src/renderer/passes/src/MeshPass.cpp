#include "MeshPass.h"

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

	CVulkanRenderer& renderer = CVulkanRenderer::get();

	CVulkanResourceManager manager;

	const SShader frag = manager.getShader("material\\mesh.frag");
	const SShader errorFrag = manager.getShader("material\\mesh_error.frag");
	const SShader vert = manager.getShader("material\\mesh.vert");

	const SShader basicFrag = manager.getShader("material\\basic.frag");
	const SShader wireframeVert = manager.getShader("material\\wireframe.vert");

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

	createInfo.vertexModule = *wireframeVert.mModule;
	createInfo.fragmentModule = *basicFrag.mModule;
	createInfo.mTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.mPolygonMode = VK_POLYGON_MODE_LINE;
	createInfo.mCullMode = VK_CULL_MODE_NONE;
	createInfo.mLineWidth = 5.f;

	wireframePipeline = renderer.mGlobalResourceManager.allocatePipeline(createInfo, attributes, CVulkanResourceManager::getBasicPipelineLayout());

	manager.flush();
}

//TODO: probably faster with gpu
bool isVisible(const Matrix4f& inViewProj, const Matrix4f& inTransformMatrix, const SBounds& inBounds) {
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

	const Matrix4f matrix = inViewProj * inTransformMatrix;

	auto min = Vector3f{std::numeric_limits<float>::max()};
	auto max = Vector3f{std::numeric_limits<float>::min()};

	for (int c = 0; c < 8; c++) {
		// project each corner into clip space
		Vector4f v = matrix * Vector4f(inBounds.origin + (corners[c] * inBounds.extents), 1.f);

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

	CVulkanRenderer& renderer = CVulkanRenderer::get();

	std::vector<std::shared_ptr<CStaticMeshObject>> renderObjects;
	{
		ZoneScopedN("Frustum Culling");
		for (const auto& renderable : CScene::get().data.objects) {
			if (renderable) {
				if (auto renderableObject = std::dynamic_pointer_cast<CStaticMeshObject>(renderable); renderableObject && renderableObject->getMesh() && isVisible(renderer.mSceneData.mViewProj, renderableObject->getTransformMatrix(), renderableObject->getMesh()->bounds)) {
					renderObjects.push_back(renderableObject);
				}
			}
		}
	}

	// Set number of meshes being drawn
	Meshes.setText(fmts("Meshes: {}", renderObjects.size()));

	// Defined outside the draw function, this is the state we will try to skip
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	uint32 drawCallCount = 0;
	uint64 vertexCount = 0;

	/*for (auto& object : renderObjects) {
		SInstancer& instancer = object->getInstancer();
		std::shared_ptr<SStaticMesh> mesh = object->getMesh();
		const size_t NumInstances = instancer.instances.size();

		ZoneScoped;
		ZoneName(object->mName.c_str(), object->mName.size());

		// Rebind index buffer if starting a new mesh
		if (lastIndexBuffer != mesh->meshBuffers.indexBuffer->buffer) {
			ZoneScopedN("Bind Buffers");

			lastIndexBuffer = mesh->meshBuffers.indexBuffer->buffer;
			vkCmdBindIndexBuffer(cmd, mesh->meshBuffers.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
			const auto offset = {
				VkDeviceSize { 0 },
				VkDeviceSize { 0 }
			};

			const auto buffers = {
				mesh->meshBuffers.vertexBuffer->buffer,
				instancer.get(object->getTransformMatrix())->buffer
			};
			vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
		}

		// Loop through surfaces and render
		for (const auto& surface : mesh->surfaces) {

			//TODO: auto pipeline rebind (or something)
			// If the materials arent the same, rebind material data
			bindPipeline(cmd, surface.material->getPipeline(renderer), surface.material->mConstants);

			vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

			drawCallCount++;
			vertexCount += surface.count * NumInstances;
		}

		//TODO: If Render Bounds

		if (CEngineLoader::getMeshes().contains("CubeBounds") && CEngineLoader::getMeshes().contains("SphereBounds")) {
			const std::shared_ptr<SStaticMesh> cubeBoundsMesh = CEngineLoader::getMeshes()["CubeBounds"];
			const std::shared_ptr<SStaticMesh> sphereBoundsMesh = CEngineLoader::getMeshes()["SphereBounds"];

			// Fill with matrix transform for bounds
			SPushConstants boxPcs;

			Transform3f transform;
			transform.mPosition = mesh->bounds.origin;
			transform.mScale = mesh->bounds.extents;
			Matrix4f mat = transform.toMatrix();

			boxPcs[0] = mat[0];
			boxPcs[1] = mat[1];
			boxPcs[2] = mat[2];
			boxPcs[3] = mat[3];
			boxPcs[4] = Vector4f(1.f, 1.f, 0.f, 1.f);

			SPushConstants spherePcs;

			transform.mScale = Vector3f(mesh->bounds.sphereRadius);
			mat = transform.toMatrix();

			spherePcs[0] = mat[0];
			spherePcs[1] = mat[1];
			spherePcs[2] = mat[2];
			spherePcs[3] = mat[3];
			spherePcs[4] = Vector4f(0.f, 0.f, 1.f, 1.f);

			// Render Bounds cube
			{
				// Bind Wireframe mesh data
				if (lastIndexBuffer != cubeBoundsMesh->meshBuffers.indexBuffer->buffer) {
					lastIndexBuffer = cubeBoundsMesh->meshBuffers.indexBuffer->buffer;
					vkCmdBindIndexBuffer(cmd, cubeBoundsMesh->meshBuffers.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
					const auto offset = {
						VkDeviceSize { 0 },
						VkDeviceSize { 0 }
					};

					const auto buffers = {
						cubeBoundsMesh->meshBuffers.vertexBuffer->buffer,
						instancer.get(object->getTransformMatrix())->buffer
					};
					vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
				}

				for (const auto& surface : cubeBoundsMesh->surfaces) {

					bindPipeline(cmd, wireframePipeline, boxPcs);

					vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

					drawCallCount++;
					vertexCount += surface.count * NumInstances;
				}
			}

			// Render Bounds sphere
			{
				// Bind Wireframe mesh data
				if (lastIndexBuffer != sphereBoundsMesh->meshBuffers.indexBuffer->buffer) {
					lastIndexBuffer = sphereBoundsMesh->meshBuffers.indexBuffer->buffer;
					vkCmdBindIndexBuffer(cmd, sphereBoundsMesh->meshBuffers.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
					const auto offset = {
						VkDeviceSize { 0 },
						VkDeviceSize { 0 }
					};

					const auto buffers = {
						sphereBoundsMesh->meshBuffers.vertexBuffer->buffer,
						instancer.get(object->getTransformMatrix())->buffer
					};
					vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
				}

				for (const auto& surface : sphereBoundsMesh->surfaces) {

					bindPipeline(cmd, wireframePipeline, spherePcs);

					vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

					drawCallCount++;
					vertexCount += surface.count * NumInstances;
				}
			}
		}
	}*/

	// Set number of drawcalls, vertices, and triangles
	Drawcalls.setText(fmts("Draw Calls: {}", drawCallCount));
	Vertices.setText(fmts("Vertices: {}", vertexCount));
	Triangles.setText(fmts("Triangles: {}", vertexCount / 3));
}
