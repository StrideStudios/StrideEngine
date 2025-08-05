#include "Viewport.h"

#include "Engine.h"
#include "Paths.h"
#include "SDL3/SDL_dialog.h"

// Some ugly code that prevents the user from having to deal with it
void sdlCallback(void* callback, const char* const* inFileName, int inFilter) {
	if (inFileName == nullptr || *inFileName == nullptr) return;
	(*reinterpret_cast<SEngineViewport::cb*>(callback))(*inFileName);
}

void SEngineViewport::queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, cb callback) {
	std::vector<SDL_DialogFileFilter> filters;
	for (auto [fst, snd] : inFilters) {
		filters.push_back({fst, snd});
	}
	SDL_ShowOpenFileDialog(sdlCallback, callback, CEngine::get().getViewport().mWindow, filters.data(), (int32)filters.size(), SPaths::get().mEnginePath, false);
}