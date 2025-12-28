#pragma once

#include <vector>
#include <memory>

#include "SceneObject.h"

class CScene : public SObject, public THierarchy<CWorldObject>, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CScene, SObject)
	MAKE_LAZY_SINGLETON(CScene)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	EXPORT virtual void update();

	TShared<class CCamera> mMainCamera = nullptr;
};