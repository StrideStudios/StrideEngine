#pragma once

#include "rendercore/VulkanResources.h"

class CMaterial;
class CVulkanSwapchain;

// Class used to house textures for the engine, easily resizable when necessary
class CEngineTextures : public SObject, public IDestroyable {

	REGISTER_CLASS(CEngineTextures, SObject)

	friend class CVulkanRenderer;

public:

	CEngineTextures() = default;
	EXPORT CEngineTextures(CRenderer* renderer);

	EXPORT void initializeTextures();

	EXPORT void reallocate(bool inUseVSync = true);

	EXPORT virtual void destroy() override;

	no_discard CVulkanSwapchain* getSwapchain() const { return m_Swapchain; }

	//
	// Textures
	//

	SImage_T* mDrawImage;
	SImage_T* mDepthImage;

	//
	// Default Data
	//

	SImage_T* mErrorCheckerboardImage;

	CMaterial* mErrorMaterial = nullptr;

private:

	//
	// SwapChain
	//

	CVulkanSwapchain* m_Swapchain;

};