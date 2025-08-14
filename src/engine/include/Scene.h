#pragma once

#include <vector>
#include <memory>

#include "Archive.h"

class CSceneObject;

class CScene : public IDestroyable {

public:

	CScene() {
		std::filesystem::path path = SPaths::get().mAssetPath.string() + "Scene.scn";

		if (!std::filesystem::exists(path)) return;

		CFileArchive file(path.string(), "rb");
		file >> data.objects;
		file.close();
	}

	virtual void destroy() override {
		std::filesystem::path path = SPaths::get().mAssetPath.string() + "Scene.scn";

		CFileArchive file(path.string(), "wb");
		file << data.objects;
		file.close();

		data.objects.clear();
	}

	struct Data {
		std::vector<std::shared_ptr<CSceneObject3D>> objects{};
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