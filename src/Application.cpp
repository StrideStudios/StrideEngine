#include "Scene.h"

//TODO: required for vulkan module to operate properly (is not a good solution)
// (DLLs would always require this with some sort of module manager)
#include "renderer/VulkanRenderer.h"
#include "SDL3/SDL_timer.h"
#include "EngineSettings.h"
#include "EngineTime.h"
#include "Input.h"
#include "Viewport.h"

//TODO: for archives i want to have base class object instead (for multiple uses) and do the factory pattern
// https://www.codeproject.com/Articles/363338/Factory-Pattern-in-Cplusplus
// https://adtmag.com/articles/2000/09/25/industrial-strength-pluggable-factories.aspx

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(int32, UseFrameCap, 180, 0, 500);
ADD_TEXT(FrameRate);
ADD_TEXT(AverageFrameRate);
ADD_TEXT(GameTime);
ADD_TEXT(DeltaTime);
#undef SETTINGS_CATEGORY

static CResourceManager gEngineResourceManager;

void init();
void run();
void update();
void end();

int main() {

    // Call to modulemanager that can force all modules load? (like a function it calls idk)

	//CEngine::get().init();
	init();

	CScene::get().init();

	//CEngine::get().run();
	run();

	CScene::get().destroy();

	//CEngine::get().end();
	end();

	return 0;
}

void init() {
	ZoneScopedN("Engine Initialization");

	astsOnce(CEngine)

	// Initialize the viewport
	gEngineResourceManager.push(CEngineViewport::get());

	// Create the renderer
	gEngineResourceManager.push<CVulkanRenderer>(CVulkanRenderer::get());
}

void run() {
	auto previousTime = std::chrono::high_resolution_clock::now();
	bool bPauseRendering = false;
	bool bRunning = true;
	while (bRunning) {
		ZoneScopedN("Update");

		// Update time
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			SEngineTime::get().mDeltaTime = std::chrono::duration<double>(currentTime - previousTime).count();

			SEngineTime::get().mFrameRate = static_cast<int32>(1.0 / SEngineTime::get().mDeltaTime);

			SEngineTime::get().mAverageFrameRate = static_cast<int32>((SEngineTime::get().mAverageFrameRate + SEngineTime::get().mFrameRate) / 2.0);

			SEngineTime::get().mGameTime += SEngineTime::get().mDeltaTime;

			previousTime = currentTime;
		}

		DeltaTime.setText(fmts("Delta Time: {}", std::to_string(SEngineTime::get().mDeltaTime)));
		FrameRate.setText(fmts("Frame Rate: {}", std::to_string(SEngineTime::get().mFrameRate)));
		AverageFrameRate.setText(fmts("Average Frame Rate: {}", std::to_string(SEngineTime::get().mAverageFrameRate)));
		GameTime.setText(fmts("Game Time: {}", std::to_string(SEngineTime::get().mGameTime)));

		// Tick input
		CInput::tick();

		// Handle SDL events
		CEngineViewport::get()->pollEvents(bRunning, bPauseRendering);

		// Do not draw if we are minimized
		if (bPauseRendering) {
			// Throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// TODO: Update the camera

		// Run the game loop
		CThreading::getGameThread().run([] {
			update();
		});

		// Draw to the screen
		CVulkanRenderer::get()->render();

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
void update() {
	ZoneScopedN("Game Loop");
	// Game logic
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void end() {

	// Wait for the gpu to finish instructions
	if (!CVulkanRenderer::get()->wait()) {
		errs("Engine did not stop properly!");
	}

	// Flush Engine Resources
	gEngineResourceManager.flush();

	// Stop 'main thread'
	CThreading::getMainThread().stop();
}