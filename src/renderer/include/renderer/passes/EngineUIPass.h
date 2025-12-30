#pragma once

#include "rendercore/Pass.h"

class CEngineUIPass final : public CPass {

	REGISTER_PASS(CEngineUIPass, CPass)

public:

	EXPORT virtual void init(TFrail<CRenderer> inRenderer) override;

	EXPORT virtual void begin() override;

	EXPORT virtual void render(const SRendererInfo& info, VkCommandBuffer cmd) override;

	EXPORT virtual void destroy() override;

	//TODO: pass resources?
private:

	TUnique<CDescriptorPool> imguiPool = nullptr;

};