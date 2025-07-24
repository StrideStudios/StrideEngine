#pragma once

#include <memory>

#include "Common.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"

class CVulkanDevice;
class CVulkanRenderer;
struct SDL_Window;

constexpr static SChar gEngineName = text("Stride Engine");

struct SEngineWindow {
	// The extent of the window
	Extent32u mExtent = {1920, 1080};

	// The SDL window
	SDL_Window* mWindow = nullptr;

	// Vulkan window surface
	VkSurfaceKHR mVkSurface;
};

class CEngine {

public:

	CEngine() = default;

	constexpr static CEngine& get() {
		static CEngine engine;
		return engine;
	}

	constexpr static const vkb::Instance& instance() {
		return get().m_Device->getInstance();
	}

	constexpr static const vkb::Device& device() {
		return get().m_Device->getDevice();
	}

	constexpr static const vkb::PhysicalDevice& physicalDevice() {
		return get().m_Device->getPhysicalDevice();
	}

	constexpr static CVulkanRenderer& renderer() {
		return *get().m_Renderer;
	}

	no_discard int32 getFrameRate() const { return m_FrameRate; }

	no_discard double getDeltaTime() const { return m_DeltaTime; }

	no_discard double getTime() const { return m_GameTime; }

	no_discard const SEngineWindow& getWindow() const { return m_EngineWindow; }

	std::string mSourcePath;
	std::string mShaderPath;
	std::string mAssetPath;

private:

	// Make sure only main can access initialization and run functions
	friend int main();

	void init();

	void end();

	void run();

	//
	// Frame Time
	//

	int32 m_AverageFrameRate = 0;

	int32 m_FrameRate = 0;

	double m_DeltaTime = 0.0;

	double m_GameTime = 0.0;

	// SDL Window
	SEngineWindow m_EngineWindow;

	//
	// Vulkan
	//

    std::unique_ptr<CVulkanDevice> m_Device = nullptr;

    std::unique_ptr<CVulkanRenderer> m_Renderer = nullptr;

};