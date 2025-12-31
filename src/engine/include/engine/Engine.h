#pragma once

#include "basic/core/Common.h"
#include "rendercore/Renderer.h"

class CInput;

class CEngine : public SObject {

	REGISTER_CLASS(CEngine, SObject)

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	EXPORT static TFrail<CEngine> get();

	EXPORT CEngine();
	EXPORT ~CEngine();

	no_discard Time& getTime() { return m_Time; }

	no_discard TFrail<CEngineViewport> getViewport() { return m_EngineViewport; }

	no_discard TFrail<CInput> getInput() { return m_Input; }

	no_discard TFrail<CRenderer> getRenderer() { return m_Renderer; }

private:

	// Make sure only main can access init and run functions
	friend int main();

	EXPORT void run_internal();

	template <typename TType>
	requires std::is_base_of_v<CRenderer, TType>
	static void run() {
		get()->m_Renderer = TUnique<TType>{};
		get()->run_internal();
	}

	EXPORT void update();

	Time m_Time{};

	TUnique<CEngineViewport> m_EngineViewport = nullptr;

	TUnique<CInput> m_Input = nullptr;

	TUnique<CRenderer> m_Renderer = nullptr;

	TUnique<CScene> m_Scene = nullptr;

};