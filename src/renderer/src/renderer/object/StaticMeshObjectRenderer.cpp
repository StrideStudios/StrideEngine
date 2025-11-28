#include "renderer/object/StaticMeshObjectRenderer.h"

#include "rendercore/Pass.h"
#include "renderer/passes/MeshPass.h"
#include "rendercore/EngineLoader.h"
#include "renderer/EngineTextures.h"
#include "renderer/VulkanRenderer.h"
#include "tracy/Tracy.hpp"

void CStaticMeshObjectRenderer::render(CMeshPass* inPass, VkCommandBuffer cmd, SRenderStack3f& stack, CStaticMeshObject* inObject, size_t& outDrawCalls, size_t& outVertices) {
	IInstancer& instancer = inObject->getInstancer();
	SStaticMesh* mesh = inObject->getMesh();
	const size_t NumInstances = instancer.getNumberOfInstances();

	ZoneScoped;
	ZoneName(inObject->mName.c_str(), inObject->mName.size());

	// Rebind index buffer if starting a new mesh
	if (m_LastIndexBuffer != mesh->meshBuffers->indexBuffer->buffer) {
		ZoneScopedN("Bind Buffers");

		m_LastIndexBuffer = mesh->meshBuffers->indexBuffer->buffer;
		vkCmdBindIndexBuffer(cmd, mesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
		const auto offset = {
			VkDeviceSize { 0 },
			VkDeviceSize { 0 }
		};

		const auto buffers = {
			mesh->meshBuffers->vertexBuffer->buffer,
			instancer.get(stack)->buffer
		};
		vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
	}

	// Loop through surfaces and render
	for (const auto& surface : mesh->surfaces) {


		const CMaterial* material = CVulkanRenderer::get()->mEngineTextures->mErrorMaterial;
		if (surface.material) {
			material = surface.material;
		}

		//TODO: auto pipeline rebind (or something)
		// If the materials arent the same, rebind material data
		inPass->bindPipeline(cmd, inPass->opaquePipeline, material->mConstants);
		//surface.material->getPipeline(renderer) TODO: pipelines
		vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);

		outDrawCalls++;
		outVertices += surface.count * NumInstances;
	}

	//TODO: If Render Bounds

	if (CEngineLoader::getMeshes().contains("CubeBounds") && CEngineLoader::getMeshes().contains("SphereBounds")) {
		const SStaticMesh* cubeBoundsMesh = CEngineLoader::getMeshes()["CubeBounds"];
		const SStaticMesh* sphereBoundsMesh = CEngineLoader::getMeshes()["SphereBounds"];

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
				vkCmdBindIndexBuffer(cmd, cubeBoundsMesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
				const auto offset = {
					VkDeviceSize { 0 },
					VkDeviceSize { 0 }
				};

				const auto buffers = {
					cubeBoundsMesh->meshBuffers->vertexBuffer->buffer,
					instancer.get(stack)->buffer
				};
				vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
			}

			for (const auto& surface : cubeBoundsMesh->surfaces) {

				inPass->bindPipeline(cmd, inPass->wireframePipeline, boxPcs);

				vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);
			}
		}

		// Render Bounds sphere
		{
			// Bind Wireframe mesh data
			if (m_LastIndexBuffer != sphereBoundsMesh->meshBuffers->indexBuffer->buffer) {
				m_LastIndexBuffer = sphereBoundsMesh->meshBuffers->indexBuffer->buffer;
				vkCmdBindIndexBuffer(cmd, sphereBoundsMesh->meshBuffers->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
				const auto offset = {
					VkDeviceSize { 0 },
					VkDeviceSize { 0 }
				};

				const auto buffers = {
					sphereBoundsMesh->meshBuffers->vertexBuffer->buffer,
					instancer.get(stack)->buffer
				};
				vkCmdBindVertexBuffers(cmd, 0, static_cast<uint32>(buffers.size()), buffers.begin(), offset.begin());
			}

			for (const auto& surface : sphereBoundsMesh->surfaces) {

				inPass->bindPipeline(cmd, inPass->wireframePipeline, spherePcs);

				vkCmdDrawIndexed(cmd, surface.count, (uint32)NumInstances, surface.startIndex, 0, 0);
			}
		}
	}
}
