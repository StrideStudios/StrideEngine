#pragma once

#include "basic/core/Common.h"
#include "sstl/Array.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

typedef struct VkInstance_T* VkInstance;
typedef struct VkDevice_T* VkDevice;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;

class CPass;

/*
 * Base classes for engine access
 */

class CSwapchain : public SObject, public TDirtyable<> {};

struct SRendererInfo {
	TFrail<class CScene> scene = nullptr;
	TFrail<class CEngineViewport> viewport = nullptr;
	TFrail<class CRenderer> renderer = nullptr;
};

class CRenderer : public SObject, public IInitializable, public IDestroyable {

public:

	/*template <typename TType>
	static void create() {
		TType* renderer = new TType();
		set(renderer);
	}*/

	EXPORT static TFrail<CRenderer> get();

	template <typename TPass>
	void addPass() {
		if (hasPass<TPass>()) return;
		m_Passes.push_back(TUnique<TPass>{this});
	}

	template <typename... TPasses>
	void addPasses() {
		(addPass<TPasses>(), ...);
	}

	template <typename TPass>
	bool hasPass() {
		return hasPass(TPass::staticClass());
	}

	template <typename TPass>
	TPass* getPass() {
		return static_cast<TPass*>(getPass(TPass::staticClass()));
	}

	virtual void immediateSubmit(std::function<void(TFrail<class CVRICommands>)>&& function) = 0;

	virtual void render(SRendererInfo& info) = 0;

	virtual bool wait() = 0;

	std::list<TUnique<CPass>>& getPasses() { return m_Passes; }

	const std::list<TUnique<CPass>>& getPasses() const { return m_Passes; }

	EXPORT virtual void destroy() override;

private:

	friend class CEngine;

	EXPORT CPass* getPass(const SClass* cls) const;

	EXPORT bool hasPass(const SClass* cls) const;

	std::list<TUnique<CPass>> m_Passes;

};