#pragma once

#include "Pass.h"

class CEngineUIPass final : public CPass {

	REGISTER_PASS(CEngineUIPass)

public:

	EXPORT virtual void init() override;

	EXPORT virtual void begin() override;

	EXPORT virtual void render(VkCommandBuffer cmd) override;

	EXPORT virtual void destroy() override;

};