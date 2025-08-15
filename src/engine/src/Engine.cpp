#include "Engine.h"

#include "EngineLoader.h"
#include "EngineSettings.h"
#include "Threading.h"
#include "Viewport.h"
#include "Camera.h"
#include "Input.h"
#include "Scene.h"
#include "SectionManager.h"
#include "SDL3/SDL_timer.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(int32, UseFrameCap, 180, 0, 500);
ADD_TEXT(FrameRate);
ADD_TEXT(AverageFrameRate);
ADD_TEXT(GameTime);
ADD_TEXT(DeltaTime);
#undef SETTINGS_CATEGORY

static CResourceManager gEngineResourceManager;

CEngine& CEngine::get() {
	static CEngine engine;
	return engine;
}

CScene& CEngine::scene() {
	return *get().m_Scene;
}

void CEngine::init() {
	ZoneScopedN("Engine Initialization");

	astsOnce(CEngine)

	// Initialize the viewport
	gEngineResourceManager.push(m_EngineViewport);

	// Create the renderer
	//gEngineResourceManager.push<CTestRenderer>(m_Renderer);

	m_Renderer = CSectionManager::getSection<CRendererSection>("renderer");
	gEngineResourceManager.add(m_Renderer);

	// Create the scene
	gEngineResourceManager.push(m_Scene);

	// Create the camera
	gEngineResourceManager.push(mMainCamera);
}

void CEngine::end() {

	// Wait for the gpu to finish instructions
	if (!m_Renderer->wait()) {
		errs("Engine did not stop properly!");
	}

	// Flush Engine Resources
	gEngineResourceManager.flush();

	// Stop 'main thread'
	CThreading::getMainThread().stop();
}

void CEngine::run() {
	auto previousTime = std::chrono::high_resolution_clock::now();
	bool bPauseRendering = false;
	bool bRunning = true;
	while (bRunning) {
		ZoneScopedN("Update");

		// Update time
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			m_Time.mDeltaTime = std::chrono::duration<double>(currentTime - previousTime).count();

			m_Time.mFrameRate = static_cast<int32>(1.0 / m_Time.mDeltaTime);

			m_Time.mAverageFrameRate = static_cast<int32>((m_Time.mAverageFrameRate + m_Time.mFrameRate) / 2.0);

			m_Time.mGameTime += m_Time.mDeltaTime;

			previousTime = currentTime;
		}

		DeltaTime.setText(fmts("Delta Time: {}", std::to_string(m_Time.mDeltaTime)));
		FrameRate.setText(fmts("Frame Rate: {}", std::to_string(m_Time.mFrameRate)));
		AverageFrameRate.setText(fmts("Average Frame Rate: {}", std::to_string(m_Time.mAverageFrameRate)));
		GameTime.setText(fmts("Game Time: {}", std::to_string(m_Time.mGameTime)));

		// Tick input
		CInput::tick();

		// Handle SDL events
		m_EngineViewport->pollEvents(bRunning, bPauseRendering);

		// Do not draw if we are minimized
		if (bPauseRendering) {
			// Throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Update the camera
		mMainCamera->update();

		// Run the game loop
		CThreading::getGameThread().run([this] {
			update();
		});

		// Draw to the screen
		renderer().render();

		// Execute any tasks that are on the 'main thread'
		// Done here because they can be done during the frame cap wait
		CThreading::getMainThread().executeAll();

		// If we go over the target framerate, delay
		// Ensure no divide by 0
		if (UseFrameCap.get() > 0) {
			ZoneScopedN("Frame Cap Wait");

			const double TargetDeltaTime = 1.0 / UseFrameCap.get();
			if (const auto frameTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count(); TargetDeltaTime > frameTime) {
				SDL_Delay(static_cast<uint32>((TargetDeltaTime - frameTime) * 1000.0));
			}
		}
	}
}

// Test game loop
void CEngine::update() {
	ZoneScopedN("Game Loop");
	// Game logic
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
