#pragma once

#include <set>
#include <vulkan/vulkan_core.h>

#include "VulkanResourceManager.h"

#define DEFINE_PASS(className) \
	className(): CPass(#className) {} \
	static CPass* make() { \
		className* pass = new className(); \
		return pass; \
	}

class EXPORT CPass : public IInitializable<>, public IDestroyable {

public:

	CPass(): m_Name("Empty Pass") {}

	CPass(const std::string& inPassName): m_Name(inPassName) {}

	virtual void render(VkCommandBuffer cmd) = 0;

	std::string getName() const { return m_Name; }

	void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

	void beginRendering(VkCommandBuffer cmd, Extent32u inExtent, const SImage_T* inColorImage = nullptr, const SImage_T* inDepthImage = nullptr, const SImage_T* inStencilImage = nullptr) const;

	bool hasSameRenderingInfo(const CPass* inOther) const;

protected:

	virtual VkRenderingFlagBits getRenderingInfoFlags() const { return static_cast<VkRenderingFlagBits>(0); }

	virtual bool isResolvePass() const { return false; }

	virtual SRenderAttachment getColorAttachment() const { return SRenderAttachment::defaultColor(); }

	virtual SRenderAttachment getDepthAttachment() const { return SRenderAttachment::defaultDepth(); }

	virtual SRenderAttachment getStencilAttachment() const { return SRenderAttachment::defaultStencil(); }

private:

	std::string m_Name = "Pass";

	CPipeline* m_CurrentPipeline = nullptr;
};
