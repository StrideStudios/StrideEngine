#pragma once

#include <vector>
#include <memory>

#include "SceneObject.h"

class CScene : public IInitializable, public IDestroyable {

	MAKE_SINGLETON(CScene)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	EXPORT virtual void update();

	class CCamera* mMainCamera = nullptr;

	struct Data {
		std::vector<std::shared_ptr<CWorldObject>> objects{};
	};

	Data data{};

	friend CArchive& operator<<(CArchive& inArchive, const CScene& inScene) {
		inArchive << inScene.data.objects;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CScene& inScene) {
		inArchive >> inScene.data.objects;
		return inArchive;
	}
};