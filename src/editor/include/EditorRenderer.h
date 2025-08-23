#pragma once

#include "renderer/VulkanRenderer.h"

class EXPORT CEditorRenderer :public CVulkanRenderer {

public:

	virtual void init() override;
};