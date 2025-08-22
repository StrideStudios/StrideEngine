#pragma once

#include "core/Common.h"

// Forward declare vkb types
namespace vkb {
	struct Instance;
	struct Device;
	struct PhysicalDevice;
}

/*
 * Base classes for engine access
 */

class CInstance {

public:

	virtual ~CInstance() = default;

	no_discard virtual const vkb::Instance& getInstance() const = 0;

};

class CDevice {

public:

	virtual ~CDevice() = default;

	no_discard virtual const vkb::PhysicalDevice& getPhysicalDevice() const = 0;

	no_discard virtual const vkb::Device& getDevice() const = 0;

};

class CSwapchain {

public:

	virtual ~CSwapchain() = default;

	virtual bool isDirty() const = 0;

	virtual void setDirty() = 0;

};

class CRenderer : public IInitializable<>, public IDestroyable {

public:

	EXPORT static CRenderer* get();

	EXPORT static void set(CRenderer* inRenderer);

	static const vkb::Instance& instance() { return get()->getInstance()->getInstance(); }

	static const vkb::PhysicalDevice& physicalDevice() { return get()->getDevice()->getPhysicalDevice(); }

	static const vkb::Device& device() { return get()->getDevice()->getDevice(); }

	no_discard virtual CInstance* getInstance() = 0;

	no_discard virtual CDevice* getDevice() = 0;

	no_discard virtual CSwapchain* getSwapchain() = 0;

	virtual void immediateSubmit(std::function<void(struct SCommandBuffer& cmd)>&& function) = 0;

	virtual void render() = 0;

	no_discard virtual bool wait() = 0;

};