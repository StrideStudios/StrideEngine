#pragma once

#include <filesystem>

#include "StaticMesh.h"

struct SLoadedGLTF;

// TODO: automatically load meshes, and have some members for that
class CMeshLoader {
public:
	static std::optional<std::shared_ptr<SLoadedGLTF>> loadGLTF(class CVulkanRenderer* renderer, class CGPUScene* GPUscene, std::filesystem::path filePath);
};
