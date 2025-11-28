#pragma once

#include "renderer/object/ObjectRenderer.h"
#include "scene/world/StaticMeshObject.h"
#include "renderer/passes/MeshPass.h"

class CStaticMeshObjectRenderer : public CWorldObjectRenderer<CStaticMeshObject, CMeshPass> {

	REGISTER_RENDERER(CStaticMeshObjectRenderer, CStaticMeshObject, CWorldObjectRenderer<CStaticMeshObject, CMeshPass>)

public:

	virtual void begin() override {
		m_LastIndexBuffer = VK_NULL_HANDLE;
	}

	EXPORT virtual void render(CMeshPass* inPass, VkCommandBuffer cmd, SRenderStack3f& stack, CStaticMeshObject* inObject, size_t& outDrawCalls, size_t& outVertices) override;

private:

	VkBuffer m_LastIndexBuffer = VK_NULL_HANDLE;

};
