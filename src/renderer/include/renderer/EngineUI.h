#pragma once

#include "rendercore/Pass.h"

class CEngineUIPass final : public CPass {

	REGISTER_PASS(CEngineUIPass, CPass)

public:

	EXPORT virtual void init(CRenderer* inRenderer) override;

	EXPORT virtual void begin() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	EXPORT virtual void destroy() override;

};