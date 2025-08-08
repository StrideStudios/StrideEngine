#pragma once

#include <vector>
#include <memory>

#include "Archive.h"

class CScene {

public:
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