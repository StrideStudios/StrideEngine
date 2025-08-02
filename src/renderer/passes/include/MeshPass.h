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

	void init(EMeshPass inPassType) override;

	void render(VkCommandBuffer cmd);

	void push(const std::set<std::shared_ptr<SStaticMesh>>& inObjects) {
		objects.insert_range(inObjects);
	}

	EMeshPass passType;

	// Surfaces used to render this pass
	std::set<std::shared_ptr<SStaticMesh>> objects;

	//
	// Pipelines
	//

	SMaterialPipeline opaquePipeline;
	SMaterialPipeline errorPipeline;
	SMaterialPipeline transparentPipeline;
};