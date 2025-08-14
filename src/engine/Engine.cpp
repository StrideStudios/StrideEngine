#include "Engine.h"

#include "EngineLoader.h"
#include "EngineSettings.h"
#include "TestRenderer.h"
#include "Threading.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "Viewport.h"
#include "Camera.h"
#include "Input.h"
#include "Scene.h"
#include "SDL3/SDL_timer.h"

#define SETTINGS_CATEGORY "Engine"
ADD_COMMAND(int32, UseFrameCap, 180, 0, 500);
ADD_TEXT(FrameRate);
ADD_TEXT(AverageFrameRate);
ADD_TEXT(GameTime);
ADD_TEXT(DeltaTime);
#undef SETTINGS_CATEGORY

int main() {
	CEngine::get().init();

	CEngine::get().run();

	CEngine::get().end();

	CThreading::getMainThread().stop();

	return 0;
}

static CResourceManager gEngineResourceManager;

CEngine::CEngine() = default;

CEngine::~CEngine() = default;

const vkb::Instance& CEngine::instance() {
	return get().m_Instance->mInstance;
}

const vkb::Device& CEngine::device() {
	return get().m_Device->getDevice();
}

const vkb::PhysicalDevice& CEngine::physicalDevice() {
	return get().m_Device->getPhysicalDevice();
}

CScene& CEngine::scene() {
	return *get().m_Scene;
}

void CEngine::init() {
	ZoneScopedN("Engine Initialization");

	astsOnce(CEngine)

	// Initializes the vkb instance
	gEngineResourceManager.push(m_Instance);

	// Initialize the viewport
	gEngineResourceManager.push(m_EngineViewport);

	// Create the vulkan device
	gEngineResourceManager.push(m_Device);

	// Create the renderer
	gEngineResourceManager.push<CTestRenderer>(m_Renderer);

	// Load textures and meshes
	CEngineLoader::load();

	// Create the scene
	gEngineResourceManager.push(m_Scene);

	// Create the camera
	gEngineResourceManager.push(mMainCamera);
}

void CEngine::end() {

	// Wait for the gpu to finish instructions
	if (!m_Renderer->waitForGpu()) {
		errs("Engine did not stop properly!");
	}

	// Flush Engine Resources
	gEngineResourceManager.flush();
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

		// It's hard to read at 180 hz, so update every tenth frame (or 18 hz)
		if (m_Renderer->getFrameNumber() % 10 == 0) {
			DeltaTime.setText(fmts("Delta Time: {}", std::to_string(m_Time.mDeltaTime)));
			FrameRate.setText(fmts("Frame Rate: {}", std::to_string(m_Time.mFrameRate)));
			AverageFrameRate.setText(fmts("Average Frame Rate: {}", std::to_string(m_Time.mAverageFrameRate)));
			GameTime.setText(fmts("Game Time: {}", std::to_string(m_Time.mGameTime)));
		}

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
		renderer().draw();

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
