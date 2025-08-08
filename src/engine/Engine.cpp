#include "Engine.h"

#include <filesystem>
#include <thread>
#include <tracy/Tracy.hpp>

#include "EngineLoader.h"
#include "EngineSettings.h"
#include "EngineTextures.h"
#include "imgui_impl_sdl3.h"
#include "Input.h"
#include "Paths.h"
#include "ResourceManager.h"
#include "TestRenderer.h"
#include "Threading.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_vulkan.h"
#include "tracy/TracyC.h"

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

CEngine::CEngine() = default;

CEngine::~CEngine() = default;

const vkb::Instance& CEngine::instance() {
	return get().m_Device->getInstance();
}

const vkb::Device& CEngine::device() {
	return get().m_Device->getDevice();
}

const vkb::PhysicalDevice& CEngine::physicalDevice() {
	return get().m_Device->getPhysicalDevice();
}

void CEngine::init() {
	ZoneScopedN("Engine Initialization");

	astsOnce(CEngine)

	// Initialize SDL3
	SDL_Init(SDL_INIT_VIDEO);

	// Create the SDL window, let it know it should be with vulkan and resizable
	m_EngineViewport.mWindow = SDL_CreateWindow(
		gEngineName,
		static_cast<int32>(m_EngineViewport.mExtent.x),
		static_cast<int32>(m_EngineViewport.mExtent.y),
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

	// Create the vulkan device, which also initializes the vkb instance
	m_Device = std::make_unique<CVulkanDevice>();

	// Create a surface for Device to reference
	SDL_Vulkan_CreateSurface(m_EngineViewport.mWindow, instance(), nullptr, &m_EngineViewport.mVkSurface);

	// Initialize the device and physical device
	m_Device->initDevice();

	// Create the renderer
	m_Renderer = std::make_unique<CTestRenderer>();

	// Initialize the allocator
	CResourceManager::init();

	// Initialize the renderer
	m_Renderer->init();

	CEngineLoader::load();
}

void CEngine::end() {

	// Wait for the gpu to finish instructions
	if (!m_Renderer->waitForGpu()) {
		errs("Engine did not stop properly!");
	}

	// Tell the renderer to destroy
	m_Renderer->destroy();

	// Destroy allocator, if not all allocations have been destroyed, it will throw an error
	CResourceManager::destroyAllocator();

	// Destroy surface, device, and viewport
	vkb::destroy_surface(instance(), m_EngineViewport.mVkSurface);
	m_Device->destroy();
	SDL_DestroyWindow(m_EngineViewport.mWindow);
}

void CEngine::run() {

	SDL_Event e;
	bool pauseRendering = false;

	auto previousTime = std::chrono::high_resolution_clock::now();

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

		// Handle events on queue
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					bRunning = false;
					// Stop all processes
					//TODO: keep rendering and have icon to tell user it's stopping
					CThreading::stop();
					break;
				case SDL_EVENT_WINDOW_MINIMIZED:
					pauseRendering = true;
					break;
				case SDL_EVENT_WINDOW_RESTORED:
					pauseRendering = false;
					break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_MAXIMIZED:
					m_EngineViewport.update({static_cast<uint32>(e.window.data1), static_cast<uint32>(e.window.data2)});
					m_Renderer->mEngineTextures->getSwapchain().setDirty();
					break;
				default: break;
			}

			ImGui_ImplSDL3_ProcessEvent(&e);

			// ImGui will consume input
			if (!ImGui::IsAnyItemActive()) {
				CInput::process(e);
			}

			// If Escape or End pressed, end the program
			if (CInput::getKeyPressed(EKey::ESCAPE) || CInput::getKeyPressed(EKey::END)) {
				bRunning = false;
			}

			// If we shouldn't show the mouse, make it impossible to leave the viewport
			SDL_SetWindowRelativeMouseMode(m_EngineViewport.mWindow, !mMainCamera.mShowMouse);
		}

		// Do not draw if we are minimized
		if (pauseRendering) {
			// Throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Update the camera
		mMainCamera.update();

		// Run the game loop
		CThreading::getGameThread().run([this] {
			update();
		});

		// Draw to the screen
		renderer().draw();

		// Execute any tasks that are on the 'main thread'
		// Done here because they might be done during the frame cap wait
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

void CEngine::update() {
	ZoneScopedN("Game Loop");
	// Game logic
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
