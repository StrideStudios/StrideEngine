#pragma once

#include "renderer/object/ObjectRenderer.h"
#include "scene/world/StaticMeshObject.h"
#include "renderer/passes/MeshPass.h"

class CStaticMeshObjectRenderer : public CWorldObjectRenderer<CStaticMeshObject, CMeshPass> {

	REGISTER_RENDERER(CStaticMeshObjectRenderer, CStaticMeshObject, CWorldObjectRenderer<CStaticMeshObject, CMeshPass>)

public:

	typedef class {

	} Proxy;

	virtual void begin() override {
		m_LastIndexBuffer = VK_NULL_HANDLE;
	}

	EXPORT virtual void render(const SRendererInfo& info, CMeshPass* inPass, const TFrail<CVRICommands>& cmd, SRenderStack3f& stack, CStaticMeshObject* inObject, size_t& outDrawCalls, size_t& outVertices) override;

private:

	VkBuffer m_LastIndexBuffer = VK_NULL_HANDLE;

};
