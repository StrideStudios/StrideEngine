#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <memory>

#include "Material.h"
#include "Paths.h"

struct SStaticMesh;

class CEngineLoader {

	static CEngineLoader& get() {
		static CEngineLoader meshLoader;
		return meshLoader;
	}

	template <typename TType>
	static void save(const std::string& inFileName, const TType& inValue) {
		std::filesystem::path path = SPaths::get().mAssetCachePath;
		path.append(inFileName);

		CFileArchive file(path.string(), "wb");

		file << inValue;

		file.close();
	}

	template <typename TType>
	static TType load(const std::filesystem::path& inPath) {
		CFileArchive file(inPath.string(), "rb");

		TType value;

		file >> value;

		file.close();

		return value;
	}

public:

	CEngineLoader() = default;

	static void save();

	static void load();

	//
	// Textures
	//

	static void importTexture(const std::filesystem::path& inPath);

	static std::map<std::string, SImage> getImages() { return get().mImages; }

	std::map<std::string, SImage> mImages{};

	//
	// Materials
	//

	static void createMaterial(const std::string& inMaterialName);

	static std::map<std::string, std::shared_ptr<CMaterial>> getMaterials() { return get().mMaterials; }

	std::map<std::string, std::shared_ptr<CMaterial>> mMaterials{};

	//
	// Materials
	//

	static void importMesh(const std::filesystem::path& inPath);

	static std::map<std::string, std::shared_ptr<SStaticMesh>> getMeshes() { return get().mMeshes; }

	std::map<std::string, std::shared_ptr<SStaticMesh>> mMeshes{};

};
