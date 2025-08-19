#pragma once

class EXPORT CEngineViewport : public IInitializable<>, public IDestroyable {

public:

	CEngineViewport() = default;

	static const CEngineViewport& get();

	typedef void cb(std::vector<std::string> inFiles);

	static void queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, cb callback);

	virtual void init() override;

	virtual void destroy() override;

	void pollEvents(bool& outRunning, bool& outPauseRendering);

	void update(const Extent32u inExtent) {
		mExtent = inExtent;
	}

	// The extent of the window
	Extent32u mExtent = {1920, 1080};

	// The SDL window
	struct SDL_Window* mWindow = nullptr;

};
