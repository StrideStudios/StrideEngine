#pragma once

#include "basic/core/Common.h"
#include "basic/core/Singleton.h"

class CVulkanRenderer;
class CEngineViewport;
class CScene;

class CEngine : public SObject, public IInitializable {

	REGISTER_CLASS(CEngine, SObject)
	MAKE_LAZY_SINGLETON(CEngine)

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	EXPORT virtual void init() override;

	no_discard Time& getTime() { return m_Time; }

	no_discard const CEngineViewport& getViewport() const { return *m_EngineViewport; }

private:

	// Make sure only main can access init and run functions
	friend int main();

	EXPORT void run();

	EXPORT void update();

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	CEngineViewport* m_EngineViewport = nullptr;

};