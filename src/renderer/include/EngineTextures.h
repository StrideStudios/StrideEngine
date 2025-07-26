#pragma once

#include "ResourceManager.h"
#include "Swapchain.h"

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures : public IDestroyable {

	friend class CVulkanRenderer;

public:

	CEngineTextures();

	void initializeTextures();

	void reallocate(bool inUseVSync = true);

	void destroy() override;

	CSwapchain& getSwapchain() { return m_Swapchain; }

	//
	// Textures
	//

	SImage mDrawImage;
	SImage mDepthImage;

	//
	// Descriptors
	//

	VkDescriptorSet mDrawImageDescriptors;
	VkDescriptorSetLayout mDrawImageDescriptorLayout;

private:

	CResourceManager m_ResourceManager;

	//
	// SwapChain
	//

	CSwapchain m_Swapchain;

};