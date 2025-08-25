#include "base/Scene.h"

#include "world/Camera.h"

static CScene gScene;
static CResourceManager gSceneResourceManager;

CScene& CScene::get() {
	return gScene;
}

void CScene::init() {
	std::filesystem::path path = SPaths::get().mAssetPath.string() + "Scene.scn";

	if (!std::filesystem::exists(path)) return;

	CFileArchive file(path.string(), "rb");
	file >> data.objects;
	file.close();

	gSceneResourceManager.create(mMainCamera);
}

void CScene::destroy() {
	gSceneResourceManager.flush();

	std::filesystem::path path = SPaths::get().mAssetPath.string() + "Scene.scn";

	CFileArchive file(path.string(), "wb");
	file << data.objects;
	file.close();

	data.objects.clear();
}

void CScene::update() {
	mMainCamera->update();
}
