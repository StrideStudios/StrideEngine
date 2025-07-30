#pragma once

#include <set>

#include "Material.h"
#include "StaticMesh.h"

enum class EMeshPass : uint8 {
	BASE_PASS
};

class CVulkanRenderer;
class CGPUScene;

struct SMeshPass {

	SMeshPass(EMeshPass inPassType): passType(inPassType) {}

	void build(CVulkanRenderer* renderer, CGPUScene* gpuScene);

	void render(const CVulkanRenderer* renderer, VkCommandBuffer cmd);

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
	SMaterialPipeline transparentPipeline;
};