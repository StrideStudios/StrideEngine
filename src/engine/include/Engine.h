#pragma once

#include <memory>

#include "Common.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

class CCamera;
struct SVulkanInstance;
class CVulkanDevice;
class CVulkanRenderer;
class CEngineViewport;
class CScene;

constexpr static auto gEngineName = text("Stride Engine");

class CEngine {

	friend CVulkanDevice;//TODO: rmv

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

	static CScene& scene();

	constexpr static CVulkanRenderer& renderer() {
		return *get().m_Renderer;
	}

	no_discard Time& getTime() { return m_Time; }

	no_discard const CEngineViewport& getViewport() const { return *m_EngineViewport; }

	CCamera* mMainCamera = nullptr;

private:

	// Make sure only main can access init and run functions
	friend int main();

	void init();

	void end();

	void run();

	void update();

	//
	// Scenes
	//

	CScene* m_Scene;

	//
	// Frame Time
	//

	Time m_Time{};

	// SDL Window
	CEngineViewport* m_EngineViewport = nullptr;

	//
	// Vulkan
	//

	SVulkanInstance* m_Instance = nullptr;

    CVulkanDevice* m_Device = nullptr;

	CVulkanRenderer* m_Renderer = nullptr;

};