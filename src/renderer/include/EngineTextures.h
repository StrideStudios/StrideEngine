#pragma once

#include "Common.h"
#include "ResourceManager.h"
#include "VulkanResourceManager.h"

class CMaterial;
class CSwapchain;

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures : public IDestroyable, public IInitializable<> {

	friend class CVulkanRenderer;

public:

	CEngineTextures() = default;

	virtual void init() override;

	void initializeTextures();

	void reallocate(bool inUseVSync = true);

	virtual void destroy() override;

	no_discard CSwapchain& getSwapchain() const { return *m_Swapchain; }

	//
	// Textures
	//

	SImage_T* mDrawImage;
	SImage_T* mDepthImage;

	//
	// Default Data
	//

	SImage_T* mErrorCheckerboardImage;

	std::shared_ptr<CMaterial> mErrorMaterial = nullptr;

private:

	//
	// SwapChain
	//

	CSwapchain* m_Swapchain;

};