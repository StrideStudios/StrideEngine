#include "scene/base/Scene.h"

#include "scene/world/Camera.h"

void CScene::init() {
	std::filesystem::path path = SPaths::get()->mAssetPath.string() + "Scene.scn";

	if (std::filesystem::exists(path)) {
		CFileArchive file(path.string(), "rb");
		file >> *this;
		file.close();
	}

	mMainCamera = TShared<CCamera>{};
}

void CScene::destroy() {
	mMainCamera.destroy();

	std::filesystem::path path = SPaths::get()->mAssetPath.string() + "Scene.scn";

	CFileArchive file(path.string(), "wb");
	file << *this;
	file.close();
}

void CScene::update() {
	mMainCamera->update();
}
