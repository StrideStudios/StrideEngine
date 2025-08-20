#pragma once

#include <set>
#include <vulkan/vulkan_core.h>

#include "VulkanResourceManager.h"

#define DEFINE_PASS(className) \
	className(): CPass(#className) {} \
	static void enable(CResourceManager& manager) { \
		astsOnce(className); \
		CPass* pass; \
		manager.create<className>(pass); \
		CPass::addPass(pass); \
	}

class EXPORT CPass : public IInitializable<>, public IDestroyable {

public:

	CPass(): m_Name("Empty Pass") {}

	CPass(const std::string& inPassName): m_Name(inPassName) {}

	static std::set<CPass*>& getPasses();

	virtual void render(VkCommandBuffer cmd) = 0;

	std::string getName() const { return m_Name; }

	void bindPipeline(VkCommandBuffer cmd, CPipeline* inPipeline, const struct SPushConstants& inConstants);

protected:

	static void addPass(CPass* pass);

private:

	std::string m_Name = "Pass";

	CPipeline* m_CurrentPipeline = nullptr;
};
