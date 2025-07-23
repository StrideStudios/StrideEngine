#pragma once

#include <memory>

#include "Common.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"

class CVulkanDevice;
class CVulkanRenderer;
struct SDL_Window;

// Typedef from SDL3 vulkan to prevent include
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

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

	static CEngine& get() {
		static CEngine engine;
		return engine;
	}

	void init();

	void end();

	void run();

	no_discard int32 getFrameRate() const { return m_FrameRate; }

	no_discard double getDeltaTime() const { return m_DeltaTime; }

	no_discard double getTime() const { return m_GameTime; }

	no_discard const SEngineWindow& getWindow() const { return m_EngineWindow; }

	no_discard CVulkanDevice& getDevice() { return *m_Device; }

	no_discard const CVulkanDevice& getDevice() const { return *m_Device; }

	no_discard CVulkanRenderer& getRenderer() { return *m_Renderer; }

	no_discard const CVulkanRenderer& getRenderer() const { return *m_Renderer; }

	std::string mSourcePath;
	std::string mShaderPath;

private:

	//
	// Frame Time
	//

	int32 m_AverageFrameRate = 0;

	int32 m_FrameRate = 0;

	double m_DeltaTime = 0.0;

	double m_GameTime = 0.0;

	//
	// SDL
	//

	SEngineWindow m_EngineWindow;

	bool m_PauseRendering = false;

	//
	// Vulkan
	//

    std::unique_ptr<CVulkanDevice> m_Device = nullptr;

    std::unique_ptr<CVulkanRenderer> m_Renderer = nullptr;

};