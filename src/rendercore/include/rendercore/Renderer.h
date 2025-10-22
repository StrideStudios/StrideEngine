#pragma once

#include "basic/control/ResourceManager.h"
#include "basic/core/Common.h"

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

class CInstance : public SObject {

public:

	no_discard virtual const vkb::Instance& getInstance() const = 0;

};

class CDevice : public SObject {

public:

	no_discard virtual const vkb::PhysicalDevice& getPhysicalDevice() const = 0;

	no_discard virtual const vkb::Device& getDevice() const = 0;

};

class CSwapchain : public SObject, public TDirtyable<> {};

class CRenderer : public SObject, public IInitializable, public IDestroyable {

public:

	EXPORT static CRenderer* get();

	template <typename TType>
	static void create() {
		TType* renderer = new TType();
		set(renderer);
	}

	template <typename... TPasses>
	static void addPasses(TPasses... passes) {
		get()->m_Passes.append_range(std::initializer_list<CPass*>{passes...});
	}

	static const vkb::Instance& instance() { return get()->getInstance()->getInstance(); }

	EXPORT static const VkInstance& vkInstance();

	static const vkb::PhysicalDevice& physicalDevice() { return get()->getDevice()->getPhysicalDevice(); }

	EXPORT static const VkPhysicalDevice& vkPhysicalDevice();

	static const vkb::Device& device() { return get()->getDevice()->getDevice(); }

	EXPORT static const VkDevice& vkDevice();

	no_discard virtual CInstance* getInstance() = 0;

	no_discard virtual CDevice* getDevice() = 0;

	no_discard virtual CSwapchain* getSwapchain() = 0;

	virtual void immediateSubmit(std::function<void(struct SCommandBuffer& cmd)>&& function) = 0;

	virtual void render() = 0;

	virtual bool wait() = 0;

	std::list<CPass*> getPasses() const { return m_Passes; }

private:

	EXPORT static void set(CRenderer* inRenderer);

	std::list<CPass*> m_Passes;

};