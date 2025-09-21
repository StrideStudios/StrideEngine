#pragma once

#include "VulkanResources.h"

class CEngineViewport : public SObject, public IInitializable, public IDestroyable {

	REGISTER_CLASS(CEngineViewport, SObject)

public:

	CEngineViewport() = default;

	EXPORT static const CEngineViewport& get();

	typedef void FCallback(std::vector<std::string> inFiles);

	EXPORT static void queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, FCallback* callback);

	EXPORT virtual void init() override;

	EXPORT virtual void destroy() override;

	EXPORT void pollEvents(bool& outRunning, bool& outPauseRendering);

	void update(const Extent32u inExtent) {
		mExtent = inExtent;
	}

	void set(VkCommandBuffer cmd) const {
		const VkViewport viewport = {
			.x = 0,
			.y = 0,
			.width = (float)mExtent.x,
			.height = (float)mExtent.y,
			.minDepth = 0.f,
			.maxDepth = 1.f
		};


		vkCmdSetViewport(cmd, 0, 1, &viewport);

		const VkRect2D scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = mExtent.x,
				.height = mExtent.y
			}
		};

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	// The extent of the window
	Extent32u mExtent = {1920, 1080};

	// The SDL window
	struct SDL_Window* mWindow = nullptr;

};
