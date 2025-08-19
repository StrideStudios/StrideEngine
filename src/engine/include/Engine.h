#pragma once

class CVulkanRenderer;
class CEngineViewport;
class CScene;

constexpr static auto gEngineName = text("Stride Engine");

class CEngine {

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	CEngine() = default;

	EXPORT static CEngine& get();

	no_discard Time& getTime() { return m_Time; }

	no_discard const CEngineViewport& getViewport() const { return *m_EngineViewport; }

public:

	// Make sure only main can access init and run functions
	friend int main();

	EXPORT void init();

	EXPORT void end();

	EXPORT void run();

	void update();

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	CEngineViewport* m_EngineViewport = nullptr;

};