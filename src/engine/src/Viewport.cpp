#include "Viewport.h"

#include "Engine.h"
#include "imgui_impl_sdl3.h"
#include "Input.h"
#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_init.h"
#include "VkBootstrap.h"

CEngineViewport*& CEngineViewport::get() {
	static CEngineViewport* viewport;
	return viewport;
}

// Some ugly code that prevents the user from having to deal with it
void sdlCallback(void* callback, const char* const* inFileName, int inFilter) {
	if (inFileName == nullptr || *inFileName == nullptr) return;

	std::vector<std::string> fileNames;
	while (*inFileName != nullptr) {
		fileNames.push_back(*inFileName);
		++inFileName;
	}
	(*reinterpret_cast<CEngineViewport::cb*>(callback))(fileNames);
}

void CEngineViewport::queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, cb callback) {
	std::vector<SDL_DialogFileFilter> filters;
	for (auto [fst, snd] : inFilters) {
		filters.push_back({fst, snd});
	}
	SDL_ShowOpenFileDialog(sdlCallback, callback, get()->mWindow, filters.data(), (int32)filters.size(), SPaths::get().mEnginePath.parent_path().string().c_str(), true);
}

void CEngineViewport::init() {
	// Initialize SDL3
	SDL_Init(SDL_INIT_VIDEO);

	// Create the SDL window, let it know it should be with vulkan and resizable
	mWindow = SDL_CreateWindow(
		gEngineName,
		static_cast<int32>(mExtent.x),
		static_cast<int32>(mExtent.y),
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);

}

void CEngineViewport::destroy() {
	SDL_DestroyWindow(mWindow);
}

void CEngineViewport::pollEvents(bool& outRunning, bool& outPauseRendering) {
	static SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_EVENT_QUIT:
				outRunning = false;
				// Stop all processes
				//TODO: keep rendering and have icon to tell user it's stopping
				CThreading::stop();
			case SDL_EVENT_WINDOW_MINIMIZED:
				outPauseRendering = true;
				break;
			case SDL_EVENT_WINDOW_RESTORED:
				outPauseRendering = false;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_MAXIMIZED:
				update({static_cast<uint32>(e.window.data1), static_cast<uint32>(e.window.data2)});
				//CVulkanRenderer::get().mEngineTextures->getSwapchain().setDirty();
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
			outRunning = false;
		}

		// If we shouldn't show the mouse, make it impossible to leave the viewport
		SDL_SetWindowRelativeMouseMode(mWindow, !CInput::shouldShowMouse());
	}
}
