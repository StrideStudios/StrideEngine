#pragma once

#include "renderer/object/ObjectRenderer.h"
#include "world/StaticMeshObject.h"
#include "passes/MeshPass.h"

class CStaticMeshObjectRenderer : public CWorldObjectRenderer<CStaticMeshObject, CMeshPass> {

	REGISTER_RENDERER(CStaticMeshObjectRenderer)

public:

	virtual void begin() override {
		m_LastIndexBuffer = VK_NULL_HANDLE;
	}

	EXPORT virtual void render(CMeshPass* inPass, VkCommandBuffer cmd, CStaticMeshObject* inObject, uint32& outDrawCalls, uint64& outVertices) override;

private:

	VkBuffer m_LastIndexBuffer = VK_NULL_HANDLE;

};
