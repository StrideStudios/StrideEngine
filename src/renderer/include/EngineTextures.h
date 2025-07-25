#pragma once

#include "ResourceManager.h"
#include "Swapchain.h"

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures {

	friend class CVulkanRenderer;

public:

	CEngineTextures();

	~CEngineTextures();

	void initializeTextures();

	void reallocate();

	void destroy();

	CSwapchain& getSwapchain() { return m_Swapchain; }

	//
	// Textures
	//

	SImage mDrawImage;
	SImage mDepthImage;

private:

	CResourceManager m_ResourceManager;

	//
	// SwapChain
	//

	CSwapchain m_Swapchain;

};