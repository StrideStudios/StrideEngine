#pragma once

#include <filesystem>

#include "StaticMesh.h"

struct SLoadedGLTF;

// TODO: automatically load meshes, and have some members for that
class CMeshLoader {
public:

	static std::optional<std::vector<std::shared_ptr<SStaticMesh>>> loadStaticMesh(class CVulkanRenderer* renderer, class CEngineBuffers* buffers, std::filesystem::path filePath);

	static std::optional<std::shared_ptr<SLoadedGLTF>> loadGLTF(class CVulkanRenderer* renderer, class CGPUScene* GPUscene, std::filesystem::path filePath);
};
