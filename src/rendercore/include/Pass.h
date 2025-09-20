#pragma once

#include <vulkan/vulkan_core.h>

#include "VulkanResourceManager.h"
#include "core/Registry.h"

class CObjectRenderer;

#define REGISTER_PASS(className) \
	REGISTER_STRUCT(className)\
	REGISTER_OBJ(CPassDeferredRegistry, className)

class CPass : public SObject, public IInitializable, public IDestroyable {

public:

	CPass() {}

	virtual void render(VkCommandBuffer cmd) = 0;

	virtual void update() {}

	EXPORT void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

	EXPORT void beginRendering(VkCommandBuffer cmd, Extent32u inExtent, const SImage_T* inColorImage = nullptr, const SImage_T* inDepthImage = nullptr, const SImage_T* inStencilImage = nullptr) const;

	EXPORT bool hasSameRenderingInfo(const CPass* inOther) const;

protected:

	virtual VkRenderingFlagBits getRenderingInfoFlags() const { return static_cast<VkRenderingFlagBits>(0); }

	virtual bool isResolvePass() const { return false; }

	virtual SRenderAttachment getColorAttachment() const { return SRenderAttachment::defaultColor(); }

	virtual SRenderAttachment getDepthAttachment() const { return SRenderAttachment::defaultDepth(); }

	virtual SRenderAttachment getStencilAttachment() const { return SRenderAttachment::defaultStencil(); }

	std::map<std::string, std::shared_ptr<CObjectRenderer>> m_Renderers;

private:

	CPipeline* m_CurrentPipeline = nullptr;
};

DEFINE_DEFERRED_REGISTRY(CPass)