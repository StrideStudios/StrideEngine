#pragma once

#include <filesystem>

#include "StaticMesh.h"

class CMeshLoader {
public:
	static std::optional<std::vector<std::shared_ptr<SStaticMesh>>> loadStaticMesh(class CVulkanRenderer* renderer, class CEngineBuffers* engineBuffers, std::filesystem::path filePath);
};
