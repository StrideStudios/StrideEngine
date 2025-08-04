#pragma once

#include <memory>
#include <vulkan/vulkan_core.h>

#include "Camera.h"
#include "Common.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

class CVulkanDevice;
class CVulkanRenderer;
struct SDL_Window;

constexpr static auto gEngineName = text("Stride Engine");

struct SEngineWindow {

	typedef void cb(const char* fileName);

	// The extent of the window
	Extent32u mExtent = {1920, 1080};

	// The SDL window
	SDL_Window* mWindow = nullptr;

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;

	static void queryForFile(const std::vector<std::pair<const char*, const char*>>& inFilters, cb callback);
};

class CEngine {

	struct Time {
		int32 mAverageFrameRate = 0;
		int32 mFrameRate = 0;
		double mDeltaTime = 0.0;
		double mGameTime = 0.0;
	};

public:

	CEngine();

	~CEngine();

	constexpr static CEngine& get() {
		static CEngine engine;
		return engine;
	}

	static const vkb::Instance& instance();

	static const vkb::Device& device();

	static const vkb::PhysicalDevice& physicalDevice();

	constexpr static CVulkanRenderer& renderer() {
		return *get().m_Renderer;
	}

	no_discard Time getTime() const { return m_Time; }

	no_discard const SEngineWindow& getWindow() const { return m_EngineWindow; }

	std::string mEnginePath;
	std::string mSourcePath;
	std::string mShaderPath;
	std::string mAssetPath;
	std::string mCachePath;
	std::string mAssetCachePath;

	CCamera mMainCamera;

private:

	// Make sure only main can access init and run functions
	friend int main();

	void init();

	void end();

	void run();

	//
	// Frame Time
	//

	Time m_Time;

	// SDL Window
	SEngineWindow m_EngineWindow;

	//
	// Vulkan
	//

    std::unique_ptr<CVulkanDevice> m_Device = nullptr;

    std::unique_ptr<CVulkanRenderer> m_Renderer = nullptr;

};