#pragma once

#include <filesystem>
#include <set>

#include "ResourceManager.h"

struct SStaticMesh;

// TODO: automatically load meshes, and have some members for that
class CMeshLoader : public IDestroyable {
public:

	CMeshLoader() = default;

	void loadGLTF(class CVulkanRenderer* renderer, std::filesystem::path filePath);

	std::set<std::shared_ptr<SStaticMesh>> mLoadedModels{};
};
