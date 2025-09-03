#pragma once

#include "renderer/object/ObjectRenderer.h"
#include "StaticMesh.h"
#include "passes/MeshPass.h"

class CStaticMeshObjectRenderer : public CWorldObjectRenderer<CStaticMeshObject, CMeshPass> {
public:
	virtual void render(CMeshPass* inPass, VkCommandBuffer cmd, CStaticMeshObject* inObject, uint32& outDrawCalls, uint64& outVertices) override;
};
