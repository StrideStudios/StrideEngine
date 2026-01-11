#include "renderer/object/StaticMeshObjectRenderer.h"

#include "rendercore/Pass.h"
#include "renderer/passes/MeshPass.h"
#include "rendercore/EngineLoader.h"
#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"
#include "tracy/Tracy.hpp"
#include "VRI/VRICommands.h"

void CStaticMeshObjectRenderer::render(const SRendererInfo& info, CMeshPass* inPass, const TFrail<CVRICommands>& cmd, SRenderStack3f& stack, CStaticMeshObject* inObject, size_t& outDrawCalls, size_t& outVertices) {
	IInstancer& instancer = inObject->getInstancer();
	SStaticMesh* mesh = inObject->getMesh();
	if (!mesh) return;
	const size_t NumInstances = instancer.getNumberOfInstances();

	ZoneScoped;
	ZoneName(inObject->mName.c_str(), inObject->mName.size());

	// Rebind index buffer if starting a new mesh
	if (m_LastIndexBuffer != mesh->meshBuffers->indexBuffer->buffer) {
		ZoneScopedN("Bind Buffers");

		m_LastIndexBuffer = mesh->meshBuffers->indexBuffer->buffer;
		cmd->bindIndexBuffers(mesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
		const auto offset = {
			VkDeviceSize { 0 },
			VkDeviceSize { 0 }
		};

		const auto buffers = {
			mesh->meshBuffers->vertexBuffer->buffer,
			instancer.get(stack)->buffer
		};
		cmd->bindVertexBuffers(0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
	}

	// Loop through surfaces and render
	for (const auto& surface : mesh->surfaces) {


		const CMaterial* material = info.renderer.staticCast<CVulkanRenderer>()->mEngineTextures->mErrorMaterial.get();
		if (surface.material) {
			material = surface.material;
		}

		//TODO: auto pipeline rebind (or something)
		// If the materials arent the same, rebind material data
		inPass->bindPipeline(cmd, inPass->opaquePipeline.get(), material->mConstants);
		//surface.material->getPipeline(renderer) TODO: pipelines
		cmd->drawIndexed(surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

		outDrawCalls++;
		outVertices += surface.count * NumInstances;
	}

	//TODO: If Render Bounds

	if (CEngineLoader::getMeshes().contains("CubeBounds") && CEngineLoader::getMeshes().contains("SphereBounds")) {
		const SStaticMesh* cubeBoundsMesh = CEngineLoader::getMeshes()["CubeBounds"].get();
		const SStaticMesh* sphereBoundsMesh = CEngineLoader::getMeshes()["SphereBounds"].get();

		// Fill with matrix transform for bounds
		SPushConstants boxPcs;

		Transform3f transform;
		transform.setPosition(mesh->bounds.origin);
		transform.setScale(mesh->bounds.extents);
		Matrix4f mat = transform.toMatrix();

		boxPcs[0] = mat[0];
		boxPcs[1] = mat[1];
		boxPcs[2] = mat[2];
		boxPcs[3] = mat[3];
		boxPcs[4] = Vector4f(1.f, 1.f, 0.f, 1.f);

		SPushConstants spherePcs;

		transform.setScale(Vector3f(mesh->bounds.sphereRadius));
		mat = transform.toMatrix();

		spherePcs[0] = mat[0];
		spherePcs[1] = mat[1];
		spherePcs[2] = mat[2];
		spherePcs[3] = mat[3];
		spherePcs[4] = Vector4f(0.f, 0.f, 1.f, 1.f);

		// Render Bounds cube
		{
			// Bind Wireframe mesh data
			if (m_LastIndexBuffer != cubeBoundsMesh->meshBuffers->indexBuffer->buffer) {
				m_LastIndexBuffer = cubeBoundsMesh->meshBuffers->indexBuffer->buffer;
				cmd->bindIndexBuffers(cubeBoundsMesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
				const auto offset = {
					VkDeviceSize { 0 },
					VkDeviceSize { 0 }
				};

				const auto buffers = {
					cubeBoundsMesh->meshBuffers->vertexBuffer->buffer,
					instancer.get(stack)->buffer
				};
				cmd->bindVertexBuffers(0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
			}

			for (const auto& surface : cubeBoundsMesh->surfaces) {

				inPass->bindPipeline(cmd, inPass->wireframePipeline.get(), boxPcs);

				cmd->drawIndexed(surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);
			}
		}

		// Render Bounds sphere
		{
			// Bind Wireframe mesh data
			if (m_LastIndexBuffer != sphereBoundsMesh->meshBuffers->indexBuffer->buffer) {
				m_LastIndexBuffer = sphereBoundsMesh->meshBuffers->indexBuffer->buffer;
				cmd->bindIndexBuffers(sphereBoundsMesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
				const auto offset = {
					VkDeviceSize { 0 },
					VkDeviceSize { 0 }
				};

				const auto buffers = {
					sphereBoundsMesh->meshBuffers->vertexBuffer->buffer,
					instancer.get(stack)->buffer
				};
				cmd->bindVertexBuffers(0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
			}

			for (const auto& surface : sphereBoundsMesh->surfaces) {

				inPass->bindPipeline(cmd, inPass->wireframePipeline.get(), spherePcs);

				cmd->drawIndexed(surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);
			}
		}
	}
}
