#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "Common.h"
#include "ResourceManager.h"

class CEngineViewport : public IInitializable<>, public IDestroyable {

public:

	CEngineViewport() = default;

	typedef void cb(std::vector<std::string> inFiles);

	static void queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, cb callback);

	virtual void init() override;

	virtual void destroy() override;

	void pollEvents(bool& outRunning, bool& outPauseRendering);

	void set(const VkCommandBuffer cmd) const {
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)mExtent.x;
		viewport.height = (float)mExtent.y;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = mExtent.x;
		scissor.extent.height = mExtent.y;

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void update(const Extent32u inExtent) {
		mExtent = inExtent;
	}

	// The extent of the window
	Extent32u mExtent = {1920, 1080};

	// The SDL window
	struct SDL_Window* mWindow = nullptr;

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;

};
