#pragma once

#include <vulkan/vulkan_core.h>

#include "rendercore/VulkanResources.h"

class CVRICommands;
class CObjectRenderer;

#define REGISTER_PASS(className, parentName) \
	REGISTER_STRUCT(className, parentName)

class CPass : public SObject, public TInitializable<TFrail<CRenderer>>, public IDestroyable {

public:

	CPass() {}

	virtual void onAdded() {}

	virtual void begin() {}

	virtual void end() {}

	virtual void render(const SRendererInfo& info, const TFrail<CVRICommands>& cmd) = 0;

	virtual void update() {}

	EXPORT void bindPipeline(const TFrail<CVRICommands>& cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

	EXPORT void beginRendering(const TFrail<CVRICommands>& cmd, Extent32u inExtent, const TFrail<SVRIImage>& inColorImage = nullptr, const TFrail<SVRIImage>& inDepthImage = nullptr, const TFrail<SVRIImage>& inStencilImage = nullptr) const;

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