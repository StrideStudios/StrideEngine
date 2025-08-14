#pragma once

#include <set>

#include "Material.h"
#include "Pass.h"
#include "StaticMesh.h"

enum class EMeshPass : uint8 {
	BASE_PASS
};

class CVulkanRenderer;
class CGPUScene;

class CMeshPass : public CPass, public IInitializable<EMeshPass> {

public:

	virtual void init(EMeshPass inPassType) override;

	virtual void render(VkCommandBuffer cmd) override;

	EMeshPass passType;

	//
	// Pipelines
	//

	CPipeline* opaquePipeline;
	CPipeline* errorPipeline;
	CPipeline* wireframePipeline;
	CPipeline* transparentPipeline;
};