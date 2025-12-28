#pragma once

#include "basic/core/Common.h"
#include "rendercore/Renderer.h"

class CInput;

class CEngine : public SObject {

	REGISTER_CLASS(CEngine, SObject)
	//MAKE_LAZY_SINGLETON(CEngine)

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	EXPORT static TShared<CEngine> get();

	EXPORT CEngine();

	no_discard Time& getTime() { return m_Time; }

	no_discard TShared<CEngineViewport> getViewport() const { return m_EngineViewport; }

	no_discard TShared<CInput> getInput() const { return m_Input; }

	no_discard TShared<CRenderer> getRenderer() const { return m_Renderer; }

private:

	// Make sure only main can access init and run functions
	friend int main();

	EXPORT void run_internal();

	template <typename TType>
	requires std::is_base_of_v<CRenderer, TType>
	static void run() {
		get()->m_Renderer = TShared<TType>{};
		get()->run_internal();
	}

	EXPORT void update();

	TShared<CRenderer> m_Renderer = nullptr;

	TShared<CInput> m_Input = nullptr;

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	TShared<CEngineViewport> m_EngineViewport = nullptr;

};