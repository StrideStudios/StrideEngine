#pragma once

#include <vector>
#include <memory>

#include "Archive.h"

class CSceneObject;

class CScene {

public:

	CScene() {
		std::filesystem::path path = SPaths::get().mAssetCachePath.string() + "Scene.scn";

		if (!std::filesystem::exists(path)) return;

		CFileArchive file(path.string(), "rb");
		file >> data.objects;
		file.close();
	}

	~CScene() {
		std::filesystem::path path = SPaths::get().mAssetCachePath.string() + "Scene.scn";

		CFileArchive file(path.string(), "wb");
		file << data.objects;
		file.close();
	}

	constexpr static CScene& get() {
		static CScene scene;
		return scene;
	}

	struct Data {
		std::vector<std::shared_ptr<CSceneObject>> objects{};
	};

	Data data;

	friend CArchive& operator<<(CArchive& inArchive, const CScene& inScene) {
		inArchive << inScene.data.objects;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CScene& inScene) {
		inArchive >> inScene.data.objects;
		return inArchive;
	}
};