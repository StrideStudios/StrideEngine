#pragma once

#include <filesystem>

#include "StaticMesh.h"

// TODO: automatically load meshes, and have some members for that
class CMeshLoader {
public:
	static std::optional<std::vector<std::shared_ptr<SStaticMesh>>> loadStaticMesh(class CVulkanRenderer* renderer, class CEngineBuffers* engineBuffers, std::filesystem::path filePath);
};
