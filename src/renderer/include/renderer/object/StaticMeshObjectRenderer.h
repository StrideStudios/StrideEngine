#pragma once

#include "renderer/object/ObjectRenderer.h"
#include "StaticMesh.h"
#include "passes/MeshPass.h"

class CStaticMeshObjectRenderer : public CWorldObjectRenderer<CStaticMeshObject, CMeshPass> {

	REGISTER_CLASS(CStaticMeshObjectRenderer) //TODO: combine macros lol

	REGISTER_RENDERER(CStaticMeshObjectRenderer, CMeshPass)

public:

	virtual void begin() override {}

	EXPORT virtual void render(CMeshPass* inPass, VkCommandBuffer cmd, CStaticMeshObject* inObject, uint32& outDrawCalls, uint64& outVertices) override;

	virtual void end() override {}
};
