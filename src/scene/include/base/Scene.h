#pragma once

#include <filesystem>
#include <vector>
#include <memory>

#include "SceneObject.h"

class CSceneObject;

class CScene : public IInitializable<>, public IDestroyable {

public:

	EXPORT static CScene& get();

	virtual void init() override;

	virtual void destroy() override;

	virtual void update();

	class CCamera* mMainCamera = nullptr;

	struct Data {
		std::vector<std::shared_ptr<CWorldObject>> objects{};
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