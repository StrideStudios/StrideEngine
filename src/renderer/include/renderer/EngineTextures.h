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
	EXPORT CEngineTextures(const TFrail<CRenderer>& renderer, TFrail<CVulkanAllocator> allocator);

	EXPORT void initializeTextures(TFrail<CVulkanAllocator> allocator);

	EXPORT void reallocate(const SRendererInfo& info, bool inUseVSync = true);

	EXPORT virtual void destroy() override;

	no_discard TShared<CVulkanSwapchain> getSwapchain() const { return m_Swapchain; }

	//
	// Samplers
	//

	TUnique<CSampler> mNearestSampler = nullptr;

	TUnique<CSampler> mLinearSampler = nullptr;

	//
	// Default Data
	//

	TUnique<SImage_T> mErrorCheckerboardImage = nullptr;

	//
	// Textures
	//

	TUnique<SImage_T> mDrawImage = nullptr;
	TUnique<SImage_T> mDepthImage = nullptr;

	TUnique<CMaterial> mErrorMaterial = nullptr;

private:

	//
	// SwapChain
	//

	TShared<CVulkanSwapchain> m_Swapchain = nullptr;

};