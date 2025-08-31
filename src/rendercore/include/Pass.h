#pragma once

#include <set>
#include <vulkan/vulkan_core.h>

#include "VulkanResourceManager.h"

#define DEFINE_PASS(className) \
	className() = default; \
	static CPass* make() { \
		return new className(); \
	} \
	virtual std::string getName() const override { return #className; } \

class CPass : public IInitializable<>, public IDestroyable {

public:

	CPass() {}

	virtual void render(VkCommandBuffer cmd) = 0;

	virtual void update() {}

	virtual std::string getName() const { return "Pass"; } \

	EXPORT void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

	EXPORT void beginRendering(VkCommandBuffer cmd, Extent32u inExtent, const SImage_T* inColorImage = nullptr, const SImage_T* inDepthImage = nullptr, const SImage_T* inStencilImage = nullptr) const;

	EXPORT bool hasSameRenderingInfo(const CPass* inOther) const;

protected:

	virtual VkRenderingFlagBits getRenderingInfoFlags() const { return static_cast<VkRenderingFlagBits>(0); }

	virtual bool isResolvePass() const { return false; }

	virtual SRenderAttachment getColorAttachment() const { return SRenderAttachment::defaultColor(); }

	virtual SRenderAttachment getDepthAttachment() const { return SRenderAttachment::defaultDepth(); }

	virtual SRenderAttachment getStencilAttachment() const { return SRenderAttachment::defaultStencil(); }

private:

	CPipeline* m_CurrentPipeline = nullptr;
};
