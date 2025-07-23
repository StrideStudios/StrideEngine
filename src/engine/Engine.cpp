#include "Engine.h"

#include <filesystem>
#include <thread>

#include "EngineSettings.h"
#include "imgui_impl_sdl3.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_vulkan.h"

#define COMMAND_CATEGORY "Engine"
ADD_COMMAND(UseVsync, true);
ADD_COMMAND(UseFrameCap, 180, 0, 500);
#undef COMMAND_CATEGORY

int main() {
	CEngine::get().init();

	CEngine::get().run();

	CEngine::get().end();
	return 0;
}

void CEngine::init() {

	assertOnce(CEngine);

	std::filesystem::path cwd = std::filesystem::current_path();
	mSourcePath = cwd.string().append("\\");
	mShaderPath = mSourcePath.append("..\\shaders\\");

	// Initialize SDL3
	SDL_Init(SDL_INIT_VIDEO);

	m_EngineWindow.mWindow = SDL_CreateWindow(
		"Stride Engine",
		static_cast<int32>(m_EngineWindow.mExtent.x),
		static_cast<int32>(m_EngineWindow.mExtent.y),
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

	SDL_Event e;

	bool bRunning = true;
	auto previousTime = std::chrono::high_resolution_clock::now();

	bool bOldVsync = true;

	while (bRunning) {

		// Time utils
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			m_DeltaTime = std::chrono::duration<double>(currentTime - previousTime).count();

			m_FrameRate = static_cast<int32>(1.0 / m_DeltaTime);

			m_AverageFrameRate = static_cast<int32>((m_AverageFrameRate + m_FrameRate) / 2.0);

			m_GameTime += m_DeltaTime;

			previousTime = currentTime;
		}

		// Easy way to visualize FPS before text is implemented
		// It's hard to read at 180 hz, so update every tenth frame (or 18 hz)
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
					m_EngineWindow.mExtent = {static_cast<uint32>(e.window.data1), static_cast<uint32>(e.window.data2)};
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

		if (ImGui::Begin("background")) {
			SComputeEffect& selected = m_Renderer->m_BackgroundEffects[m_Renderer->m_CurrentBackgroundEffect];

			ImGui::Text("Selected effect: ", selected.name);

			ImGui::SliderInt("Effect Index", &m_Renderer->m_CurrentBackgroundEffect, 0, static_cast<int32>(m_Renderer->m_BackgroundEffects.size()) - 1);

			ImGui::InputFloat4("data1",reinterpret_cast<float*>(&selected.data.data1));
			ImGui::InputFloat4("data2",reinterpret_cast<float*>(&selected.data.data2));
			ImGui::InputFloat4("data3",reinterpret_cast<float*>(&selected.data.data3));
			ImGui::InputFloat4("data4",reinterpret_cast<float*>(&selected.data.data4));
		}
		ImGui::End();

		if (bOldVsync != UseVsync.getBool()) {
			bOldVsync = UseVsync.getBool();
			std::cout << "Reallocating Swapchain to " << (UseVsync.getBool() ? "enable VSync." : "disable VSync.") << std::endl;
			m_Renderer->m_EngineTextures->getSwapchain().recreate(UseVsync.getBool());
		}
		CEngineSettings::render();

		ImGui::Render();

		getRenderer().render();

		// If we go over the target framerate, delay
		// Ensure no divide by 0
		if (UseFrameCap.getInt() > 0) {
			const double TargetDeltaTime = 1.0 / UseFrameCap.getInt();
			if (const auto frameTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - previousTime).count(); TargetDeltaTime > frameTime) {
				SDL_Delay(static_cast<std::uint32_t>((TargetDeltaTime - frameTime) * 1000.0));
			}
		}
	}
}
