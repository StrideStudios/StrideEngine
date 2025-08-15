#pragma once

#include <memory>

#include "Common.h"

class CCamera;
class CRendererSection;
class CEngineViewport;
class CScene;

constexpr static auto gEngineName = text("Stride Engine");

#define ENGINE_API __declspec(dllexport)

class CEngine {

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	CEngine() = default;

	ENGINE_API static CEngine& get();

	ENGINE_API static CScene& scene();

	static CRendererSection& renderer() {
		return *get().m_Renderer;
	}

	no_discard Time& getTime() { return m_Time; }

	no_discard const CEngineViewport& getViewport() const { return *m_EngineViewport; }

	CCamera* mMainCamera = nullptr;

private:

	// Make sure only main can access init and run functions
	friend int main();

	ENGINE_API void init();

	ENGINE_API void end();

	ENGINE_API void run();

	void update();

	//
	// Scenes
	//

	CScene* m_Scene;

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	CEngineViewport* m_EngineViewport = nullptr;

	//
	// Vulkan
	//

	CRendererSection* m_Renderer = nullptr;

};