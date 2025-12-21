#pragma once

#include "basic/core/Common.h"
#include "basic/core/Singleton.h"
#include "rendercore/Renderer.h"

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

	no_discard TUnique<CEngineViewport>& getViewport() { return m_EngineViewport; }

	no_discard const TUnique<CEngineViewport>& getViewport() const { return m_EngineViewport; }

private:

	// Make sure only main can access init and run functions
	friend int main();

	EXPORT void run_internal();

	template <typename TType>
	requires std::is_base_of_v<CRenderer, TType>
	static void run() {
		get()->m_Renderer = TType{};
		CRenderer::set(get()->m_Renderer.get());
		get()->run_internal();
	}

	EXPORT void update();

	TShared<CRenderer> m_Renderer = nullptr;

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	TUnique<CEngineViewport> m_EngineViewport = nullptr;

};