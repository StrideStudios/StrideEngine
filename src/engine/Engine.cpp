#include "include/Engine.h"

#include <filesystem>
#include <iostream>
#include <thread>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_vulkan.h"

static auto gEngine = CEngine();

constexpr static double TARGET_FRAMERATE = 180.0;

CEngine& CEngine::get() {
	return gEngine;
}

void CEngine::init() {

	assertOnce(CEngine);

	std::filesystem::path cwd = std::filesystem::current_path();
	mSourcePath = cwd.string().append("/");
	mShaderPath = mSourcePath.append("../shaders/");

	std::cout << mSourcePath << std::endl;
	std::cout << mShaderPath << std::endl;

	// Initialize SDL3
	SDL_Init(SDL_INIT_VIDEO);

	m_EngineWindow.mWindow = SDL_CreateWindow(
		"Stride Engine",
		m_EngineWindow.mExtent.x,
		m_EngineWindow.mExtent.y,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
	assert(m_EngineWindow.mWindow);

	SDL_SetWindowResizable(m_EngineWindow.mWindow, true);

	// Create the vulkan device, which also initializes the vkb instance
	m_Device = std::make_unique<CVulkanDevice>();

	// Create a surface for Device to reference
	SDL_Vulkan_CreateSurface(m_EngineWindow.mWindow, getDevice().getInstance(), nullptr, &m_EngineWindow.mVkSurface);
	assert(m_EngineWindow.mVkSurface);

	// Initialize the device and physical device
	m_Device->initDevice();

	// Initialize the renderer
	m_Renderer = std::make_unique<CVulkanRenderer>();
}

void CEngine::end() {
	// Wait for the gpu to finish instructions
	m_Renderer->waitForGpu();

	m_Renderer->destroy();

	vkb::destroy_surface(getDevice().getInstance(), m_EngineWindow.mVkSurface);
	m_Device->destroy();

	SDL_DestroyWindow(m_EngineWindow.mWindow);
}

void CEngine::run() {

	init();

	SDL_Event e;

	bool bRunning = true;
	auto previousTime = std::chrono::high_resolution_clock::now();

	while (bRunning) {

		// Time utils
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			m_DeltaTime = std::chrono::duration<double>(currentTime - previousTime).count();

			m_FrameRate = (int32) (1.0 / m_DeltaTime);

			m_AverageFrameRate = (m_AverageFrameRate + m_FrameRate) / 2.0;

			m_GameTime += m_DeltaTime;

			previousTime = currentTime;
		}

		// Easy way to visualize FPS before text is implemented
		// Its hard to read at 180 hz, so update every tenth frame (or 18 hz)
		if (m_Renderer->getFrameNumber() % 10 == 0) {
			std::string title = "Stride Engine Rate: ";
			title.append(std::to_string(m_AverageFrameRate));
			title.append(", Dt: ");
			title.append(std::to_string(m_DeltaTime));
			title.append(", Num: ");
			title.append(std::to_string(m_Renderer->getFrameNumber()));
			title.append(", Time: ");
			title.append(std::to_string(m_GameTime));
			SDL_SetWindowTitle(getWindow().mWindow, title.c_str());
		}

		// Handle events on queue
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					bRunning = false;
					break;
				case SDL_EVENT_WINDOW_MINIMIZED:
					m_PauseRendering = true;
					break;
				case SDL_EVENT_WINDOW_RESTORED:
					m_PauseRendering = false;
					break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_MAXIMIZED:
					m_EngineWindow.mExtent = {(uint32)e.window.data1, (uint32)e.window.data2};
					break;
				default: break;
			}
			// Forward events to backend
			ImGui_ImplSDL3_ProcessEvent(&e);
		}

		// do not draw if we are minimized
		if (m_PauseRendering) {
			// throttle the speed to avoid the endless spinning
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Start the Dear ImGui frame
		//ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		ImGui::Render();

		getRenderer().render();

		// If we go over the target framerate, delay
		constexpr double TargetDeltaTime = 1.0 / TARGET_FRAMERATE;
		if (const auto frameTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count(); TargetDeltaTime > frameTime) {
			SDL_Delay(static_cast<std::uint32_t>((TargetDeltaTime - frameTime) * 1000.0));
		}
	}

	end();
}
