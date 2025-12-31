#pragma once

#include "SceneObject.h"

class CScene : public SObject, public THierarchy<CWorldObject>, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CScene, SObject)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	EXPORT virtual void update();

	TShared<class CCamera> mMainCamera = nullptr;
};